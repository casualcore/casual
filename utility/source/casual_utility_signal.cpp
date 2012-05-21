//!
//! casual_utility_signal.cpp
//!
//! Created on: May 6, 2012
//!     Author: Lazan
//!

#include "casual_utility_signal.h"
#include "casual_exception.h"

#include <signal.h>


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

			void set( int signal)
			{
				m_signal = signal;
			}

			int consume()
			{
				int temp = m_signal;
				m_signal = 0;
				return temp;
			}

		private:
			LastSignal() : m_signal( 0)
			{
				//
				// Register all the signals
				//
				signal( SIGALRM, casual_common_signal_handler);

				signal( SIGTERM, casual_common_signal_handler);
				signal( SIGKILL, casual_common_signal_handler);
				signal( SIGQUIT, casual_common_signal_handler);
				signal( SIGINT, casual_common_signal_handler);

			}
			int m_signal;
		};

		LastSignal& globalCrap = LastSignal::instance();
	}

}



void casual_common_signal_handler( int signal)
{
	local::globalCrap.set( signal);
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
				//
				// We do nothing for 'no signal' and the alarm-signal.
				//
				case 0:
				case SIGALRM:
					break;
				default:
					throw exception::SignalTerminate( strsignal( signal));
				}
			}

		}
	}
}
