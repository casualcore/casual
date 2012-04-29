//!
//! casual_isolatedunittest_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "casual_buffer.h"


namespace casual
{

	namespace buffer
	{



		TEST( casual_common, buffer_alloc)
		{
			Buffer& buffer = Holder::instance().allocate( "STRING", "", 2048);

			EXPECT_TRUE( buffer.m_size == 2048);

		}



	}
}



