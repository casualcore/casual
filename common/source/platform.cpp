//!
//! casual_utility_platform.cpp
//!
//! Created on: Jun 14, 2012
//!     Author: Lazan
//!

#include "common/platform.h"


#include <unistd.h>




namespace casual
{
	namespace common
	{
		namespace platform
		{
			namespace local
			{
				namespace
				{
					pid_type getProcessId()
					{
						static const pid_type pid = getpid();
						return pid;
					}


				}
			}

			pid_type getProcessId()
			{
				return local::getProcessId();
			}

			std::string getSignalDescription( signal_type signal)
			{
			   return strsignal( signal);
			}

		}
	}
}




