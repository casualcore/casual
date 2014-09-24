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
#include <string.h>


#include <stack>

extern "C"
{
	void casual_common_signal_handler( int signal);
}

namespace casual
{

   namespace common
   {
      namespace local
      {
         namespace
         {
            namespace signal
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
                     m_signals.push_back( signal);
                  }

                  int consume()
                  {
                     if( !m_signals.empty())
                     {
                        int temp = m_signals.front();
                        m_signals.pop_back();
                        return temp;
                     }
                     return 0;
                  }

                  void clear()
                  {
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

                  //
                  // TODO: atomic?
                  //
                  std::deque< int> m_signals;
               };


               //
               // We need to instantiate to register the signals
               //
               Cache& globalCrap = Cache::instance();

            } // signal


         } // <unnamed>
      } // local
   } // common
} // casual



void casual_common_signal_handler( int signal)
{
	casual::common::local::signal::Cache::instance().add( signal);
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
				const int signal = local::signal::Cache::instance().consume();
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
                  throw exception::signal::Terminate( strsignal( signal));
                  break;
               }
				}
			}

         void clear()
         {
            local::signal::Cache::instance().clear();

         }

			namespace posponed
         {
            void handle()
            {

            }

         } // posponed

			namespace alarm
			{
				Scoped::Scoped( Seconds timeout)
				{
					::alarm( timeout);
				}

				Scoped::~Scoped()
				{
					::alarm( 0);
				}
			}

			void alarm::set( common::platform::seconds_type timeout)
			{
			   ::alarm( timeout);
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
         } // thread


		} // signal
	} // common
} // casual



