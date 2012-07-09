//!
//! casual_error.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "casual_error.h"


#include <string.h>
#include <errno.h>

#include "casual_logger.h"

namespace casual
{
	namespace error
	{

		int handler()
		{
			try
			{
				throw;
			}
			catch( const std::exception& exception)
			{
				logger::error << "exception: " << exception.what();
			}

			return -1;
		}


		std::string stringFromErrno()
		{
			return strerror( errno);
		}

	}


}



