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

namespace local
{
	namespace
	{
		struct LastSignal
		{
			static LastSignal& instance()
			{
				static LastSignal singleton;
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

		private:
			LastSignal()
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
			   set( casual::common::platform::cSignal_Alarm);

            set( casual::common::platform::cSignal_Terminate);
            set( casual::common::platform::cSignal_Quit);
            set( casual::common::platform::cSignal_Interupt);

            set( casual::common::platform::cSignal_ChildTerminated, SA_NOCLDSTOP);
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
		LastSignal& globalCrap = LastSignal::instance();
	} // <unnamed>
} // local



void casual_common_signal_handler( int signal)
{
	local::globalCrap.add( signal);
}


namespace casual
{

	namespace common
	{
		namespace signal
		{
			void handle()
			{
				const int signal = local::LastSignal::instance().consume();
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


         void send( platform::pid_type pid, platform::signal_type signal)
         {
            if( kill( pid, signal) == -1)
            {
               log::error << "failed to send signal (" << platform::getSignalDescription( signal) << ") to pid: " << pid << " - errno: " << errno << " - "<< error::stringFromErrno() << std::endl;
            }
         }


		} // signal
	} // common
} // casual



