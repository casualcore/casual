//!
//! casual
//!

#include "common/signal.h"
#include "common/platform.h"
#include "common/exception.h"
#include "common/log.h"
#include "common/flag.h"
#include "common/internal/trace.h"
#include "common/process.h"
#include "common/chronology.h"
#include "common/memory.h"
#include "common/cast.h"



#include <signal.h>
#include <cstring>


#include <stack>
#include <thread>
#include <iomanip>
#include <iostream>

#include <sys/time.h>


namespace casual
{
   namespace common
   {
      namespace signal
      {
         namespace local
         {
            namespace
            {

               bool send( platform::pid::type pid, platform::signal::type signal)
               {

                  log::internal::debug << "signal::send pid: " << pid << " signal: " << signal << std::endl;

                  if( ::kill( pid, signal) != 0)
                  {
                     switch( errno)
                     {
                        case ESRCH:
                           log::internal::debug << "failed to send signal (" << type::string( signal) << ") to pid: " << pid << " - errno: " << errno << " - "<< error::string() << std::endl;
                           break;
                        default:
                           log::error << "failed to send signal (" << type::string( signal) << ") to pid: " << pid << " - errno: " << errno << " - "<< error::string() << std::endl;
                           break;
                     }
                     return false;
                  }
                  return true;
               }

            } // <unnamed>
         } // local
      } // signal
   } // common
} // casual


namespace casual
{

	namespace common
	{
		namespace signal
		{
         std::ostream& operator << ( std::ostream& out, signal::Type signal)
         {
            switch( signal)
            {
               case Type::alarm: return out << "alarm";
               case Type::interrupt: return out << "interrupt";
               case Type::kill: return out << "kill";
               case Type::quit: return out << "quit";
               case Type::child: return out << "child";
               case Type::terminate: return out << "terminate";
               case Type::user: return out << "user";
               case Type::pipe: return out << "pipe";
            }
            return out << cast::underlying( signal);
         }

		   namespace type
		   {

            std::string string( Type signal)
            {
               return type::string( cast::underlying( signal));
            }

            std::string string( platform::signal::type signal)
            {
               return strsignal( signal);
            }

         } // type


		   namespace local
         {
            namespace
            {

                  namespace handler
                  {


                     std::atomic< long> global_total_pending{ 0};


                     template< signal::Type signal>
                     struct basic_pending
                     {
                        static void clear() { pending = false;}
                        static std::atomic< bool> pending;
                     };
                     template< signal::Type Signal>
                     std::atomic< bool> basic_pending< Signal>::pending{ false};



                     template< signal::Type Signal>
                     void signal_callback( platform::signal::type signal)
                     {
                        if( ! basic_pending< Signal>::pending.exchange( true))
                        {
                           ++global_total_pending;
                        }
                     }


                     struct Handle
                     {

                        static Handle& instance()
                        {
                           static Handle handle;
                           return handle;
                        }


                        void handle()
                        {
                           //
                           // We assume that loading from atomic is cheaper than mutex-lock
                           //

                           if( handler::global_total_pending.load() > 0)
                           {
                              std::lock_guard< std::mutex> lock{ m_mutex};

                              //
                              // We only allow one thread at a time to actually handle the
                              // pending signals
                              //
                              // We check the total-pending again
                              //
                              if( handler::global_total_pending.load() > 0)
                              {
                                 handle( signal::mask::current());
                              }
                           }
                        }

                        void clear()
                        {
                           global_total_pending = 0;

                           m_child.clear();
                           m_terminate.clear();
                           m_quit.clear();
                           m_interrupt.clear();
                           m_alarm.clear();
                           m_user.clear();
                           m_pipe.clear();
                        }

                     private:

                        void handle( const signal::Set& current)
                        {
                           m_child.handle( current);
                           m_terminate.handle( current);
                           m_quit.handle( current);
                           m_interrupt.handle( current);
                           m_alarm.handle( current);
                           m_user.handle( current);
                           m_pipe.handle( current);
                        }

                        Handle() = default;



                        template< signal::Type Signal, typename Exception, int Flags = 0>
                        struct basic_handler
                        {
                           using pending_type = basic_pending< Signal>;

                           basic_handler()
                           {
                              //
                              // Register the signal handler for this signal
                              //

                              struct sigaction sa;

                              memory::set( sa);
                              sa.sa_handler = &signal_callback< Signal>;
                              sa.sa_flags = Flags;

                              if( sigaction( cast::underlying( Signal), &sa, nullptr) == -1)
                              {
                                 std::cerr << "failed to register handle for signal: " << Signal << " - " << error::string() << std::endl;

                                 throw std::system_error{ error::last(), std::system_category()};
                              }
                           }

                           basic_handler( const basic_handler&) = delete;
                           basic_handler operator = ( const basic_handler&) = delete;

                           void handle( const signal::Set& current)
                           {
                              //
                              // We know that this function is invoked with mutex, so we
                              // can ignore concurrency problem
                              //

                              if( pending_type::pending.load())
                              {
                                 if( ! current.exists( Signal))
                                 {
                                    //
                                    // Signal is not blocked
                                    //
                                    log::internal::debug << "signal: handling signal: " << Signal << '\n';

                                    //
                                    // We've consumed the signal
                                    //
                                    pending_type::pending.store( false);

                                    //
                                    // Decrement the global count
                                    //
                                    --handler::global_total_pending;

                                    throw Exception{};
                                 }
                              }
                           }

                           void clear()
                           {
                              basic_pending< Signal>::clear();
                           }
                        };



                        basic_handler< signal::Type::child, exception::signal::child::Terminate, SA_NOCLDSTOP> m_child;
                        basic_handler< signal::Type::terminate, exception::signal::Terminate> m_terminate;
                        basic_handler< signal::Type::quit, exception::signal::Terminate> m_quit;
                        basic_handler< signal::Type::interrupt, exception::signal::Terminate> m_interrupt;
                        basic_handler< signal::Type::alarm, exception::signal::Timeout> m_alarm;
                        basic_handler< signal::Type::user, exception::signal::User> m_user;
                        basic_handler< signal::Type::pipe, exception::signal::Pipe> m_pipe;

                        //
                        // Only for handle
                        //
                        std::mutex m_mutex;
                     };


                  } // handler

            } // <unnamed>
         } // local


         namespace local
         {
            namespace
            {

               //
               // We need to instantiate the handler globally to trigger signal-handler-registration
               //
               handler::Handle& global_handler = handler::Handle::instance();
            } // <unnamed>
         } // local

			void handle()
			{
			   local::global_handler.handle();
			}

         void clear()
         {
            local::global_handler.clear();
         }

         namespace current
         {
            long pending()
            {
               return local::handler::global_total_pending.load();
            }
         } // current


         namespace timer
         {
            namespace local
            {
               namespace
               {

                  std::chrono::microseconds convert( const itimerval& value)
                  {
                     if( value.it_value.tv_sec == 0 && value.it_value.tv_usec == 0)
                     {
                        return std::chrono::microseconds::min();
                     }

                     return std::chrono::seconds( value.it_value.tv_sec) + std::chrono::microseconds( value.it_value.tv_usec);
                  }

                  std::chrono::microseconds get()
                  {
                     itimerval old;

                     if( ::getitimer( ITIMER_REAL, &old) != 0)
                     {
                        throw exception::invalid::Argument{ "timer::get - " + error::string()};
                     }
                     return convert( old);
                  }

                  std::chrono::microseconds set( itimerval& value)
                  {
                     itimerval old;

                     if( ::setitimer( ITIMER_REAL, &value, &old) != 0)
                     {
                        throw exception::invalid::Argument{ "timer::set - " + error::string()};
                     }

                     log::internal::debug << "timer set: "
                           << value.it_value.tv_sec << "." << std::setw( 6) << std::setfill( '0') << value.it_value.tv_usec << "s - was: "
                           << old.it_value.tv_sec << "." <<  std::setw( 6) << std::setfill( '0') << old.it_value.tv_usec << "s\n";

                     return convert( old);
                  }
               } // <unnamed>
            } // local



            std::chrono::microseconds set( std::chrono::microseconds offset)
            {
               if( offset <= std::chrono::microseconds::zero())
               {
                  if( offset == std::chrono::microseconds::min())
                  {
                     //
                     // Special case == 'unset'
                     //
                     return unset();
                  }

                  //
                  // We send the signal directly
                  //
                  log::internal::debug << "timer - offset is less than zero: " << offset.count() << " - send alarm directly" << std::endl;
                  signal::send( process::id(), signal::Type::alarm);
                  return local::get();
               }
               else
               {
                  itimerval value;
                  value.it_interval.tv_sec = 0;
                  value.it_interval.tv_usec = 0;
                  value.it_value.tv_sec = std::chrono::duration_cast< std::chrono::seconds>( offset).count();
                  value.it_value.tv_usec =  (offset % std::chrono::seconds( 1)).count();

                  return local::set( value);
               }
            }

            std::chrono::microseconds get()
            {
               return local::get();
            }

            std::chrono::microseconds unset()
            {
               itimerval value;
               memory::set( value);

               return local::set( value);
            }



            Scoped::Scoped( std::chrono::microseconds timeout, const platform::time_point& now)
            {
               auto old = timer::set( timeout);

               if( old != std::chrono::microseconds::min())
               {
                  m_old = now + old;
                  log::internal::debug << "old timepoint: " << chronology::local( m_old) << std::endl;
               }
               else
               {
                  m_old = platform::time_point::min();
               }
            }

            Scoped::Scoped( std::chrono::microseconds timeout)
               : Scoped( timeout, platform::clock_type::now())
            {
            }

            Scoped::~Scoped()
            {
               if( ! m_moved)
               {
                  if( m_old == platform::time_point::min())
                  {
                     timer::unset();
                  }
                  else
                  {
                     timer::set( m_old - platform::clock_type::now());
                  }
               }
            }


            Deadline::Deadline( const platform::time_point& deadline, const platform::time_point& now)
            {
               if( deadline != platform::time_point::max())
               {
                  timer::set( deadline - now);
               }
               else
               {
                  timer::unset();
               }
            }

            Deadline::Deadline( const platform::time_point& deadline)
             : Deadline( deadline, platform::clock_type::now()) {}


            Deadline::Deadline( std::chrono::microseconds timeout, const platform::time_point& now)
             : Deadline( now + timeout, now) {}

            Deadline::Deadline( std::chrono::microseconds timeout)
             : Deadline( timeout, platform::clock_type::now()) {}


            Deadline::~Deadline()
            {
               if( ! m_moved)
               {
                  timer::unset();
               }
            }

            Deadline::Deadline( Deadline&&) = default;
            Deadline& Deadline::operator = ( Deadline&&) = default;

         } // timer


         bool send( platform::pid::type pid, Type signal)
         {
            return local::send( pid, cast::underlying( signal));
         }


         Set::Set() : Set( empty_t{})
         {

         }

         Set::Set( set::type set) : set( std::move( set))
         {

         }


         Set::Set( std::initializer_list< Type> signals) : Set( empty_t{})
         {
            for( auto&& signal : signals)
            {
               add( signal);
            }
         }

         void Set::add( Type signal)
         {
            sigaddset( &set, cast::underlying( signal));
         }

         void Set::remove( Type signal)
         {
            sigdelset( &set, cast::underlying( signal));
         }


         bool Set::exists( Type signal) const
         {
            return sigismember( &set, cast::underlying( signal)) == 1;
         }



         Set::Set( filled_t)
         {
            sigfillset( &set);
         }
         Set::Set( empty_t)
         {
            sigemptyset( &set);
         }

         std::ostream& operator << ( std::ostream& out, const Set& value)
         {
            out << "[";

            bool exists = false;
            for( auto& signal : { Type::alarm, Type::child, Type::interrupt, Type::kill, Type::pipe, Type::quit, Type::terminate, Type::user})
            {
               if( value.exists( signal))
               {
                  if( exists)
                     out << ", " << signal;
                  else
                  {
                     out << signal;
                     exists = true;
                  }
               }
            }

            return out << ']';
         }


         Set pending()
         {
            Set result;
            sigpending( &result.set);
            return result;
         }

         namespace set
         {

            signal::Set filled()
            {
               return { signal::Set::filled_t{}};
            }

            signal::Set filled( const std::vector< Type>& excluded)
            {
               auto mask = filled();

               for( auto&& signal : excluded)
               {
                  mask.remove( signal);
               }

               return mask;
            }

            signal::Set empty()
            {
               return {};
            }
         } // set

         namespace mask
         {
            signal::Set set( signal::Set mask)
            {
               pthread_sigmask( SIG_SETMASK, &mask.set, &mask.set);
               return mask;
            }

            signal::Set block( signal::Set mask)
            {
               pthread_sigmask( SIG_BLOCK, &mask.set, &mask.set);
               return mask;
            }

            signal::Set unblock( signal::Set mask)
            {
               pthread_sigmask( SIG_UNBLOCK, &mask.set, &mask.set);
               return mask;
            }

            signal::Set block()
            {
               return set( set::filled());
            }


            signal::Set current()
            {
               signal::Set mask;
               pthread_sigmask( SIG_SETMASK, nullptr, &mask.set);
               return mask;
            }

         } // mask


         namespace thread
         {

            void send( std::thread& thread, Type signal)
            {
               log::internal::debug << "signal::thread::send thread: " << thread.get_id() << " signal: " << signal << std::endl;

               send( thread.native_handle(), signal);

               //if( pthread_kill( const_cast< std::thread&>( thread).native_handle(), signal) != 0)
            }

            void send( common::thread::native::type thread, Type signal)
            {
               if( pthread_kill( thread, cast::underlying( signal)) != 0)
               {
                  log::error << "failed to send signal (" << type::string( signal) << ") to thread - errno: " << errno << " - "<< error::string() << std::endl;
               }
            }

            void send( Type signal)
            {
               log::internal::debug << "signal::thread::send current thread - signal: " << signal << std::endl;

               send( common::thread::native::current(), signal);
            }


            namespace scope
            {
               Reset::Reset( signal::Set mask) : m_mask( std::move( mask)) {}

               Reset::~Reset()
               {
                  if( ! m_moved)
                  {
                     mask::set( m_mask);
                  }
               }

               const signal::Set& Reset::previous() const
               {
                  return m_mask;
               }



               Mask::Mask( signal::Set mask) : Reset( mask::set( mask)) {}



               Block::Block() : Reset( mask::block())
               {
               }

               Block::Block( signal::Set mask) : Reset( mask::block( mask))
               {
               }

               Unblock::Unblock( signal::Set mask) : Reset( mask::unblock( mask))
               {
               }

            } // scope

         } // thread


		} // signal
	} // common
} // casual



