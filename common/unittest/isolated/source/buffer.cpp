//!
//! casual_isolatedunittest_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "common/buffer_context.h"


namespace casual
{

	namespace buffer
	{


		TEST( casual_common, buffer_allocate)
		{
			Buffer& buffer = Context::instance().allocate( "STRING", "", 2048);

			EXPECT_TRUE( buffer.size() == 2048);

			Context::instance().deallocate( buffer.raw());

		}


		TEST( casual_common, buffer_reallocate)
		{
			Buffer* buffer = &Context::instance().allocate( "STRING", "", 2048);

			EXPECT_TRUE( buffer->size() == 2048);


			buffer = &Context::instance().reallocate( buffer->raw(), 4096);

			EXPECT_TRUE( buffer->size() == 4096);


			Context::instance().deallocate( buffer->raw());

		}
	}
}



