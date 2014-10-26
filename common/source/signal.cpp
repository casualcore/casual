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
#include "common/process.h"



#include <signal.h>
#include <cstring>


#include <stack>
#include <atomic>
#include <condition_variable>
#include <thread>

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
                        m_signals.pop_back();
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
                     /*
                     signal( casual::common::platform::cSignal_Alarm, casual_common_signal_handler);

                     signal( casual::common::platform::cSignal_Terminate, casual_common_signal_handler);
                     signal( casual::common::platform::cSignal_Kill, casual_common_signal_handler);
                     signal( casual::common::platform::cSignal_Quit, casual_common_signal_handler);
                     signal( casual::common::platform::cSignal_Interupt, casual_common_signal_handler);

                     signal( casual::common::platform::cSignal_ChildTerminated, casual_common_signal_handler);
                     */
                     set( common::signal::type::alarm);

                     set( common::signal::type::terminate);
                     set( common::signal::type::quit);
                     set( common::signal::type::interupt);

                     set( common::signal::type::child, SA_NOCLDSTOP);

                     set( common::signal::type::user);
                  }

                  void set( int signal, int flags = 0)
                  {
                     struct sigaction sa;

                     memset(&sa, 0, sizeof(sa));
                     sa.sa_handler = casual_common_signal_handler;
                     sa.sa_flags = flags | SA_RESTART;

                     sigaction( signal, &sa, 0);
                  }


                  std::deque< int> m_signals;
                  std::mutex m_mutex;
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
				const int signal = local::Cache::instance().consume();
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

         void clear()
         {
            local::Cache::instance().clear();

         }

			namespace alarm
			{
				Scoped::Scoped( std::chrono::microseconds timeout)
				{
					timer::set( timeout);
				}

				Scoped::~Scoped()
				{
				   timer::unset();
				}

				void set( platform::time_point when)
            {
               auto offset = when - platform::clock_type::now();

               timer::set( offset);
            }

			}

         namespace timer
         {
            namespace local
            {
               namespace
               {
                  void set( itimerval& value)
                  {
                     log::internal::debug << "timer set to: " << value.it_value.tv_sec << "s " << value.it_value.tv_usec << "us" << std::endl;

                     if( ::setitimer( ITIMER_REAL, &value, nullptr) != 0)
                     {
                        throw exception::invalid::Argument{ "timer::set - " + error::string()};
                     }
                  }
               } // <unnamed>
            } // local
            void set( std::chrono::microseconds offset)
            {
               if( offset <= std::chrono::microseconds::zero())
               {
                  unset();
               }
               else
               {
                  itimerval value;
                  value.it_interval.tv_sec = 0;
                  value.it_interval.tv_usec = 0;
                  value.it_value.tv_sec = std::chrono::duration_cast< std::chrono::seconds>( offset).count();
                  value.it_value.tv_usec = std::chrono::duration_cast< std::chrono::microseconds>( offset % std::chrono::seconds( 1)).count();

                  local::set( value);
               }
            }

            void unset()
            {
               itimerval value;
               memset( &value, 0, sizeof( itimerval));

               local::set( value);

            }
         }


         bool send( platform::pid_type pid, type::type signal)
         {
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

         void block( type::type signal)
         {
            sigset_t mask;
            sigemptyset( &mask);
            sigaddset( &mask, signal);
            if( sigprocmask( SIG_BLOCK, &mask, nullptr) != 0)
            {
               log::error << "failed to block signal (" << type::string( signal) << ")  - " << error::string() << std::endl;
            }
         }


         void unblock( type::type signal)
         {
            sigset_t mask;
            sigemptyset( &mask);
            sigaddset( &mask, signal);
            if( sigprocmask( SIG_UNBLOCK, &mask, nullptr) != 0)
            {
               log::error << "failed to unblock signal (" << type::string( signal) << ")  - " << error::string() << std::endl;
            }
         }

         namespace thread
         {
            //!
            //! Send signal to thread
            //!
            void send( std::thread& thread, type::type signal)
            {
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



