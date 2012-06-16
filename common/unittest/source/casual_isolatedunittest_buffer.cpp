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


		TEST( casual_common, buffer_allocate)
		{
			Buffer& buffer = Holder::instance().allocate( "STRING", "", 2048);

			EXPECT_TRUE( buffer.size() == 2048);

			Holder::instance().deallocate( buffer.raw());

		}


		TEST( casual_common, buffer_reallocate)
		{
			Buffer* buffer = &Holder::instance().allocate( "STRING", "", 2048);

			EXPECT_TRUE( buffer->size() == 2048);


			buffer = &Holder::instance().reallocate( buffer->raw(), 4096);

			EXPECT_TRUE( buffer->size() == 4096);


			Holder::instance().deallocate( buffer->raw());

		}
	}
}



