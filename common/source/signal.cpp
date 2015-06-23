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



#include <signal.h>
#include <cstring>


#include <stack>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <iomanip>

#include <sys/time.h>

extern "C"
{

	void casual_child_terminate_signal_handler( int signal);
	void casual_alarm_signal_handler( int signal);
	void casual_terminate_signal_handler( int signal);
}

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
               struct Cache
               {
                  static Cache& instance()
                  {
                     static Cache singleton;
                     return singleton;
                  }

                  //!
                  //! Indicate if a signal is present, and how many is "stacked".
                  //! in the normal flow (signals are rare) we only check one atomic.
                  //!
                  std::atomic< long> signal_count;

                  std::atomic< long> alarm_count;
                  std::atomic< long> child_terminate_count;
                  std::atomic< long> terminate_count;


                  void clear()
                  {
                     signal_count = 0;

                     alarm_count = 0;
                     child_terminate_count = 0;
                     terminate_count = 0;
                  }


                  void consume( Filter filter)
                  {
                     auto count = signal_count.load(std::memory_order_relaxed);
                     if( count > 0)
                     {
                        if( child_terminate_count > 0)
                        {
                           --child_terminate_count;
                           --signal_count;

                           if( ! flag< Filter::exclude_child_terminate>( filter))
                           {
                              throw exception::signal::child::Terminate();
                           }
                        }
                        else if( alarm_count > 0)
                        {
                           --alarm_count;
                           --signal_count;

                           if( ! flag< Filter::exclude_alarm>( filter))
                           {
                              throw exception::signal::Timeout();
                           }
                        }
                        else if( terminate_count > 0)
                        {
                           --terminate_count;
                           --signal_count;

                           if( ! flag< Filter::exclude_terminate>( filter))
                           {
                              throw exception::signal::Terminate();
                           }
                        }
                     }
                  }

               private:


                  Cache()
                   : alarm_count( 0), child_terminate_count( 0), terminate_count( 0)
                  {

                     //
                     // Register all the signals
                     //


                     resgistration( &casual_child_terminate_signal_handler, common::signal::Type::child, SA_NOCLDSTOP);

                     resgistration( &casual_alarm_signal_handler, common::signal::Type::alarm);

                     //
                     // These we terminate on...
                     //
                     resgistration( &casual_terminate_signal_handler, common::signal::Type::terminate);
                     resgistration( &casual_terminate_signal_handler, common::signal::Type::quit);
                     resgistration( &casual_terminate_signal_handler, common::signal::Type::interupt);
                     resgistration( &casual_terminate_signal_handler, common::signal::Type::user);
                  }

                  template< typename F>
                  void resgistration( F function, int signal, int flags = 0)
                  {
                     struct sigaction sa;

                     memset(&sa, 0, sizeof(sa));
                     sa.sa_handler = function;
                     sa.sa_flags = flags; // | SA_RESTART;

                     sigaction( signal, &sa, 0);
                  }
               };


               //
               // We need to instantiate to register the signals
               //
               Cache& globalCrap = Cache::instance();


            } // <unnamed>
         } // local
      } // signal
   } // common
} // casual



void casual_child_terminate_signal_handler( int signal)
{
   ++casual::common::signal::local::globalCrap.child_terminate_count;
   ++casual::common::signal::local::globalCrap.signal_count;
}
void casual_alarm_signal_handler( int signal)
{
   ++casual::common::signal::local::globalCrap.alarm_count;
   ++casual::common::signal::local::globalCrap.signal_count;
}

void casual_terminate_signal_handler( int signal)
{
   ++casual::common::signal::local::globalCrap.terminate_count;
   ++casual::common::signal::local::globalCrap.signal_count;
}

namespace casual
{

	namespace common
	{
		namespace signal
		{

		   namespace type
		   {

            std::string string( type signal)
            {
               return strsignal( signal);
            }

         } // type

			void handle()
			{
				local::Cache::instance().consume( Filter::exclude_none);
			}

         void handle( Filter exclude)
         {
            local::Cache::instance().consume( exclude);
         }

         void clear()
         {
            log::internal::debug << "signal clear" << std::endl;
            local::Cache::instance().clear();

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
               memset( &value, 0, sizeof( itimerval));

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
               if( m_old == platform::time_point::min())
               {
                  timer::unset();
               }
               else
               {
                  timer::set( m_old - platform::clock_type::now());
               }
            }

         } // timer


         bool send( platform::pid_type pid, type::type signal)
         {

            log::internal::debug << "signal::send pid: " << pid << " signal: " << signal << std::endl;

            if( ::kill( pid, signal) == -1)
            {
               switch( errno)
               {
                  case ESRCH:
                     break;
                  default:
                     log::error << "failed to send signal (" << type::string( signal) << ") to pid: " << pid << " - errno: " << errno << " - "<< error::string() << std::endl;
                     break;
               }
               return false;
            }
            return true;
         }


         namespace thread
         {
            //!
            //! Send signal to thread
            //!
            void send( std::thread& thread, type::type signal)
            {
               log::internal::debug << "signal::thread::send thread: " << thread.get_id() << " signal: " << signal << std::endl;

               //if( pthread_kill( const_cast< std::thread&>( thread).native_handle(), signal) != 0)
               if( pthread_kill( thread.native_handle(), signal) != 0)
               {
                  log::error << "failed to send signal (" << type::string( signal) << ") to thread: " << thread.get_id() << " - errno: " << errno << " - "<< error::string() << std::endl;
               }

            }

            set_type block()
            {
               sigset_t set;
               sigfillset(&set);
               sigset_t result;
               pthread_sigmask(SIG_SETMASK, &set, &result);
               return result;
            }

            namespace scope
            {

               Block::Block() : m_set( block())
               {


               }
               Block::~Block()
               {
                  pthread_sigmask(SIG_SETMASK, &m_set, NULL);
               }

            } // scope

         } // thread


		} // signal
	} // common
} // casual



