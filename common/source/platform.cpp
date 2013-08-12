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


			std::string getSignalDescription( signal_type signal)
			{
			   return strsignal( signal);
			}

		}
	}
}




