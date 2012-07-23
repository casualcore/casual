//!
//! casual_utility_signal.cpp
//!
//! Created on: May 6, 2012
//!     Author: Lazan
//!

#include "casual_utility_signal.h"
#include "casual_utility_platform.h"
#include "casual_exception.h"

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
			   m_signals.push( signal);
			}

			int consume()
			{
			   if( !m_signals.empty())
			   {
			      int temp = m_signals.top();
               m_signals.pop();
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
				signal( casual::utility::platform::cSignal_Alarm, casual_common_signal_handler);

				signal( casual::utility::platform::cSignal_Terminate, casual_common_signal_handler);
				signal( casual::utility::platform::cSignal_Kill, casual_common_signal_handler);
				signal( casual::utility::platform::cSignal_Quit, casual_common_signal_handler);
				signal( casual::utility::platform::cSignal_Interupt, casual_common_signal_handler);

			}
			std::stack< int> m_signals;
		};

		//
		// We need to instantiate to register the signals
		//
		LastSignal& globalCrap = LastSignal::instance();
	}

}



void casual_common_signal_handler( int signal)
{
	local::globalCrap.add( signal);
}


namespace casual
{

	namespace utility
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
                  // We do nothing for 'no signal' and the alarm-signal.
                  //
                  break;
               case SIGALRM:
               {
                  throw exception::signal::Timeout( "Timeout occurred");
               }
               default:
               {
                  //
                  // the rest we throw on, so the rest of the application
                  // can use RAII and other paradigms to do cleaning
                  //
                  throw exception::signal::Terminate( strsignal( signal));
               }
				}
			}

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

			void alarm::set( utility::platform::seconds_type timeout)
			{
			   ::alarm( timeout);
			}

		}
	}
}



