//!
//! casual_error.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "casual_error.h"


#include <string.h>
#include <errno.h>

namespace casual
{
	namespace error
	{

		int handler()
		{
			// TODO:
			return 0;
		}


		std::string stringFromErrno()
		{
			return strerror( errno);
		}

	}


}



