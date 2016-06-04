//!
//! casual_utility_signal.cpp
//!
//! Created on: May 6, 2012
//!     Author: Lazan
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

                  if( ::kill( pid, signal) == -1)
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
               case Type::interupt: return out << "interupt";
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
               extern "C"
               {
                  void signal_callback( platform::signal::type signal);
               }

               namespace handler
               {

                  struct Type
                  {
                     Type( signal::Type signal) : signal( signal), flag( 0) {}
                     Type( signal::Type signal, int flag) : signal( signal), flag( flag) {}

                     signal::Type signal;
                     int flag;

                     operator signal::Type() const { return signal;}

                     friend std::ostream& operator << ( std::ostream& out, const Type& value)
                     {
                        //return out << "{ signal: " << value.signal << ", flag: " << value.flag << '}';
                        return out << value.signal;
                     }
                  };
               }

               template< typename F>
               void resgistration( F function, handler::Type signal)
               {
                  struct sigaction sa;

                  memory::set( sa);
                  sa.sa_handler = function;
                  sa.sa_flags = signal.flag;

                  if( sigaction( cast::underlying( signal.signal), &sa, 0) == -1)
                  {
                     throw std::system_error{ error::last(), std::system_category()};
                  }
               }

               namespace handler
               {

                  template< signal::Type Signal, typename Exception, typename Insurance = std::false_type, int flag = 0>
                  struct basic_handler
                  {

                     using insurance_type = Insurance;
                     using exception_type = Exception;

                     static handler::Type signal() { return { Signal, flag};}

                     void handle()
                     {
                        throw exception_type{};
                     }
                  };

                  using Terminate = basic_handler< signal::Type::terminate, exception::signal::Terminate, std::true_type>;
                  using Quit = basic_handler< signal::Type::quit, exception::signal::Terminate, std::true_type>;
                  using Interupt = basic_handler< signal::Type::interupt, exception::signal::Terminate, std::true_type>;

                  using Alarm = basic_handler< signal::Type::alarm, exception::signal::Timeout>;
                  using Child = basic_handler< signal::Type::child, exception::signal::child::Terminate, std::false_type, SA_NOCLDSTOP>;

                  using User = basic_handler< signal::Type::user, exception::signal::User>;

                  using Pipe = basic_handler< signal::Type::pipe, exception::signal::Pipe>;


               } // handler


               struct Handler
               {
                  ~Handler() = default;

                  static Handler& instance()
                  {
                     static Handler singleton{
                        handler::Alarm{},
                        handler::Child{},
                        handler::User{},
                        handler::Pipe{},
                        handler::Terminate{},
                        handler::Quit{},
                        handler::Interupt{},
                     };
                     return singleton;
                  }

                  Handler( const Handler&) = delete;
                  Handler& operator = ( const Handler&) = delete;

                  void handle()
                  {
                     auto count = m_pendings.load();

                     if( count > 0)
                     {
                        scope::Execute decrement{ [&](){
                           ++m_pendings;
                        }};
                        auto current = signal::mask::current();

                        log::internal::debug << "signal::Handler::handle - handler: " << *this << ", mask: " << current << '\n';

                        for( auto& handler : m_handlers)
                        {
                           handler->handle( current);
                        }
                        decrement.release();
                     }
                  }


                  void clear()
                  {
                     m_pendings.store( 0);
                     range::for_each( m_handlers, std::mem_fn( &Handler::base_handle::clear));
                     log::internal::debug << "signal::Handler::clear handler: " << *this << '\n';
                  }


                  friend std::ostream& operator << ( std::ostream& out, const Handler& value)
                  {

                     if( out)
                     {
                        out << "{ pending: [";

                        bool empty = true;
                        for( auto& handler : value.m_handlers)
                        {
                           if( handler->pending())
                           {
                              if( empty)
                              {
                                 out << handler->signal();
                                 empty = false;
                              }
                              else
                              {
                                 out << ',' << handler->signal();
                              }
                           }
                        }
                        out << "], blocked: " << signal::pending() << '}';
                     }
                     return out;
                  }

               private:

                  friend void signal_callback( platform::signal::type);

                  template< typename... Handlers>
                  Handler( Handlers&&... handlers) : Handler( create( std::forward< Handlers>( handlers)...))
                  {

                  }


                  void signal( signal::Type signal)
                  {
                     ++m_pendings;

                     for( auto& handler : m_handlers)
                     {
                        if( handler->signal() == signal)
                        {
                           handler->signal( signal);
                           return;
                        }
                     }
                  }

                  static void signal_glitch_insurance_thread(
                        const std::atomic< platform::signal::type>& pending,
                        std::atomic< bool>& insurance,
                        common::thread::native::type target)
                  {
                     //
                     // We don't want any signals at all...
                     //
                     signal::mask::block();

                     try
                     {
                        log::internal::debug << "signal glitch insurance created for signal: " << pending << '\n';

                        scope::Execute done{ [&](){
                           insurance.store( false);
                        }};

                        while( true)
                        {
                           std::this_thread::sleep_for( std::chrono::milliseconds{ 10});

                           if( ! pending)
                           {
                              //
                              // Something has consumed some signals while we was asleep,
                              // and we don't need the insurance anymore
                              // all good, and expected. We return and terminate this thread
                              //
                              return;
                           }

                           //
                           // We've got "the glitch", send the signal again.
                           //
                           log::internal::debug << "signal glitch - send signal: " << pending << " again\n";

                           signal::thread::send( target, signal::Type( pending.load()));
                        }
                     }
                     catch( ...)
                     {
                        error::handler();
                     }
                  }


                  struct base_handle
                  {
                     virtual ~base_handle() = default;
                     virtual void signal( signal::Type) = 0;
                     virtual void handle( const signal::Set&) = 0;
                     virtual void clear() = 0;
                     virtual handler::Type signal() const = 0;
                     virtual bool pending() const = 0;
                  };

                  template< typename T>
                  struct glitch_base : base_handle
                  {
                     void glitch_insurance( const std::atomic< platform::signal::type>& pending)
                     {
                        // no-op
                     }
                  };



                  template< typename H>
                  struct basic_handle : glitch_base< typename H::insurance_type>
                  {
                     using handler_type = H;

                     basic_handle( handler_type&& handler)
                        : m_handler( std::move( handler)), m_pending( 0) {}

                     basic_handle( const basic_handle&) = delete;
                     basic_handle& operator = ( const basic_handle&) = delete;


                     handler::Type signal() const override
                     {
                        return handler_type::signal();
                     }

                     void signal( signal::Type signal) override
                     {
                        if( m_pending.exchange( cast::underlying( signal)) == 0)
                        {
                           this->glitch_insurance( m_pending);
                        }
                     }

                     void handle( const signal::Set& current) override
                     {
                        auto signal = m_pending.exchange( 0);

                        if( signal)
                        {
                           if( ! current.exists( signal::Type( signal)))
                           {
                              //
                              // Signalen är inte blockad
                              //
                              log::internal::debug << "signal: handling signal: " << signal << '\n';

                              m_handler.handle();
                           }
                           else
                           {
                              m_pending.store( signal);
                           }
                        }
                     }

                     void clear() override
                     {
                        m_pending.store( 0);
                     }

                     bool pending() const override
                     {
                        return m_pending != 0;
                      }

                     handler_type m_handler;
                     std::atomic< platform::signal::type> m_pending;
                  };


                  static std::vector< std::unique_ptr< base_handle>> create()
                  {
                     return {};
                  }

                  template< typename H, typename... Handlers>
                  static std::vector< std::unique_ptr< base_handle>> create( H&& handler, Handlers&&... handlers)
                  {
                     auto result = create( std::forward< Handlers>( handlers)...);
                     std::unique_ptr< base_handle> holder{ new basic_handle< H>( std::forward< H>( handler))};
                     result.push_back( std::move( holder));
                     return result;
                  }

                  Handler( std::vector< std::unique_ptr< base_handle>> handlers)
                     : m_pendings( 0), m_handlers{ std::move( handlers)}
                  {
                     //thread::scope::Block block;

                     for( auto& handler : m_handlers)
                     {
                        local::resgistration( &local::signal_callback, handler->signal());
                     }

                  }

                  std::atomic< long> m_pendings;
                  std::vector< std::unique_ptr< base_handle>> m_handlers;

               };


               template<>
               struct Handler::glitch_base< std::true_type> : Handler::base_handle
               {
                  glitch_base() : m_insurance{ false} {}

                  void glitch_insurance( const std::atomic< platform::signal::type>& pending)
                  {
                     if( ! m_insurance.exchange( true))
                     {
                        std::thread{ &signal_glitch_insurance_thread,
                           std::ref( pending),
                           std::ref( m_insurance),
                           common::thread::native::current()}.detach();
                     }
                  }

               private:
                  std::atomic< bool> m_insurance;
               };


               void signal_callback( platform::signal::type signal)
               {
                  Handler::instance().signal( signal::Type( signal));
               }


            } // <unnamed>
         } // local







         namespace local
         {
            namespace
            {


               Handler& global_handler = Handler::instance();
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
            for( auto& signal : { Type::alarm, Type::child, Type::interupt, Type::kill, Type::pipe, Type::quit, Type::terminate, Type::user})
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



