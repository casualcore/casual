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

			EXPECT_TRUE( buffer.m_size == 2048);

			Holder::instance().deallocate( buffer.m_memory);

		}


		TEST( casual_common, buffer_reallocate)
		{
			Buffer buffer = Holder::instance().allocate( "STRING", "", 2048);

			EXPECT_TRUE( buffer.m_size == 2048);


			buffer = Holder::instance().reallocate( buffer.m_memory, 4096);

			EXPECT_TRUE( buffer.m_size == 4096);


			Holder::instance().deallocate( buffer.m_memory);

		}

                TEST( casual_common, buffer_valgrind)
                {
			int* valgrindcheck = new int;
			EXPECT_TRUE( true);
		}
	}
}



