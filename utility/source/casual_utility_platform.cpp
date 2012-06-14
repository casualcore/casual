//!
//! casual_utility_platform.cpp
//!
//! Created on: Jun 14, 2012
//!     Author: Lazan
//!

#include "casual_utility_platform.h"





namespace casual
{
	namespace utility
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

		}
	}
}




