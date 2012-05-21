//!
//! casual_utility_signal.h
//!
//! Created on: May 6, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_UTILITY_SIGNAL_H_
#define CASUAL_UTILITY_SIGNAL_H_





namespace casual
{

	namespace utility
	{
		namespace signal
		{

			//!
			//! Throws if there has been a signal received.
			//! @note does not throw on SIGALRM
			//!
			//! @throw exception::SignalTerminate
			//!
			void handle();

		}
	}
}



#endif /* CASUAL_UTILITY_SIGNAL_H_ */
