//!
//! casual_isolatedunittest_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "common/buffer/pool.h"


namespace casual
{
   namespace common
   {
      namespace buffer
      {

         /*
         TEST( casual_common, buffer_allocate)
         {
            auto buffer = Context::instance().allocate( { "X_OCTET", ""}, 2048);

            EXPECT_TRUE( buffer != nullptr);

            Context::instance().deallocate( buffer);

         }

         TEST( casual_common, buffer_reallocate)
         {
            auto buffer = Context::instance().allocate( { "X_OCTET", ""}, 2048);

            EXPECT_TRUE( buffer != nullptr);

            auto buffer2 = Context::instance().reallocate( buffer, 4096);

            EXPECT_TRUE( buffer2 != buffer);


            Context::instance().deallocate( buffer2);

         }
         */

         TEST( casual_common_buffer, pool_allocate)
         {
            auto buffer = pool::Holder::instance().allocate( { "X_OCTET", "binary"}, 1024);

            ASSERT_TRUE( buffer != nullptr);

            pool::Holder::instance().deallocate( buffer);

         }

         TEST( casual_common_buffer, pool_allocate_non_existing__throws)
         {
            EXPECT_THROW({
               pool::Holder::instance().allocate( { "non-existing", "non-existing"}, 1024);
            }, exception::xatmi::buffer::TypeNotSupported);
         }


         TEST( casual_common_buffer, pool_reallocate)
         {
            auto small = pool::Holder::instance().allocate( { "X_OCTET", "binary"}, 64);

            auto big = pool::Holder::instance().reallocate( small, 2048);

            EXPECT_TRUE( small != nullptr);
            EXPECT_TRUE( big != nullptr);
            EXPECT_TRUE( small != big);

            pool::Holder::instance().deallocate( big);

         }

         TEST( casual_common_buffer, pool_reallocate__reallocate_again_with_old__throws)
         {
            auto small = pool::Holder::instance().allocate( { "X_OCTET", "binary"}, 64);
            auto big = pool::Holder::instance().reallocate( small, 2048);

            pool::Holder::instance().deallocate( big);

            EXPECT_THROW({
               pool::Holder::instance().reallocate( small, 2048);
            }, exception::xatmi::InvalidArguments);

         }

         TEST( casual_common_buffer, pool_type)
         {
            Type type{ "X_OCTET", "binary"};
            auto buffer = pool::Holder::instance().allocate( type, 64);

            auto&& result = pool::Holder::instance().type( buffer);

            EXPECT_TRUE( type == result);

            // will log warning
            pool::Holder::instance().clear();

         }
      }
	}
}



