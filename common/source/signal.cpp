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
	void casual_common_signal_handler( int signal);
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

                  void add( int signal)
                  {
                     std::unique_lock< std::mutex> lock( m_mutex);

                     m_signals.push_back( signal);
                  }

                  int consume()
                  {
                     std::unique_lock< std::mutex> lock( m_mutex);

                     if( ! m_signals.empty())
                     {
                        int temp = m_signals.front();
                        m_signals.pop_front();
                        return temp;
                     }
                     return 0;
                  }

                  void clear()
                  {
                     std::unique_lock< std::mutex> lock( m_mutex);

                     m_signals.clear();
                  }

               private:
                  Cache()
                  {
                     //
                     // Register all the signals
                     //

                     set( common::signal::Type::alarm);

                     set( common::signal::Type::terminate);
                     set( common::signal::Type::quit);
                     set( common::signal::Type::interupt);

                     set( common::signal::Type::child, SA_NOCLDSTOP);

                     set( common::signal::Type::user);
                  }

                  void set( int signal, int flags = 0)
                  {
                     struct sigaction sa;

                     memset(&sa, 0, sizeof(sa));
                     sa.sa_handler = casual_common_signal_handler;
                     sa.sa_flags = flags; // | SA_RESTART;

                     sigaction( signal, &sa, 0);
                  }


                  std::deque< int> m_signals;
                  std::mutex m_mutex;
               };


               //
               // We need to instantiate to register the signals
               //
               Cache& globalCrap = Cache::instance();


               void dispatch( platform::signal_type signal)
               {
                  switch( signal)
                  {
                     case 0:
                        //
                        // We do nothing for 'no signal'
                        //
                        break;
                     case exception::signal::Timeout::value:
                     {
                        throw exception::signal::Timeout();
                        break;
                     }
                     case exception::signal::child::Terminate::value:
                     {
                        throw exception::signal::child::Terminate();
                        break;
                     }
                     case exception::signal::User::value:
                     {
                        throw exception::signal::User();
                        break;
                     }
                     default:
                     {
                        //
                        // the rest we throw on, so the rest of the application
                        // can use RAII and other paradigms to do cleaning
                        //
                        throw exception::signal::Terminate( type::string( signal));
                        break;
                     }
                  }


               }

            } // <unnamed>
         } // local
      } // signal
   } // common
} // casual



void casual_common_signal_handler( int signal)
{
	casual::common::signal::local::globalCrap.add( signal);
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
				local::dispatch( local::Cache::instance().consume());
			}

         void handle( const std::vector< signal::Type>& exclude)
         {
            auto signal = local::Cache::instance().consume();

            if( ! range::find( exclude, signal))
            {
               local::dispatch( signal);
            }
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
                           << old.it_value.tv_sec << "." <<  std::setw( 6) << std::setfill( '0') << old.it_value.tv_usec << "\n";

                     return convert( old);
                  }
               } // <unnamed>
            } // local



            std::chrono::microseconds set( std::chrono::microseconds offset)
            {
               if( offset <= std::chrono::microseconds::zero())
               {
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

               if( old != std::chrono::microseconds::zero())
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



