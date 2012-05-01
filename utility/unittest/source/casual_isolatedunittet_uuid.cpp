//!
//! casual_isolatedunittet_uuid.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!




#include <gtest/gtest.h>

#include "casual_utility_uuid.h"

#include <iostream>

namespace casual
{

	namespace utility
	{

		TEST( casual_utility, uuid)
		{
			uuid someUuid;

			std::cout << someUuid.getString() << std::endl;

		}

	}

}

