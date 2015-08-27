//!
//! casual_isolatedunittest_buffer.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "common/buffer/pool.h"

#include "common/message/service.h"
#include "common/marshal/binary.h"


#include "xatmi.h"

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
            auto buffer = pool::Holder::instance().allocate( buffer::type::binary(), 1024);

            ASSERT_TRUE( buffer != nullptr);

            pool::Holder::instance().deallocate( buffer);

         }

         TEST( casual_common_buffer, pool_insert)
         {
            auto buffer = pool::Holder::instance().insert( Payload{ buffer::type::binary(), 1024});

            ASSERT_TRUE( buffer != nullptr);

            pool::Holder::instance().clear();
         }

         TEST( casual_common_buffer, pool_insert__deallocate__expect__inbound_still_valid)
         {
            auto inbound = pool::Holder::instance().insert( Payload{ buffer::type::binary(), 1024});

            pool::Holder::instance().deallocate( inbound);

            EXPECT_NO_THROW({
               auto send = pool::Holder::instance().get( inbound);
               EXPECT_TRUE( send.reserved == 1024);
            });

            pool::Holder::instance().clear();
         }

         TEST( casual_common_buffer, pool_insert__clear__expect__inbound_deallocated)
         {
            auto inbound = pool::Holder::instance().insert( Payload{ buffer::type::binary(), 1024});

            pool::Holder::instance().clear();

            EXPECT_THROW({
               pool::Holder::instance().get( inbound);
            }, exception::xatmi::invalid::Argument);

         }

         TEST( casual_common_buffer, pool_allocate_non_existing__throws)
         {
            EXPECT_THROW({
               pool::Holder::instance().allocate( { "non-existing", "non-existing"}, 1024);
            }, exception::xatmi::buffer::type::Input);
         }


         TEST( casual_common_buffer, pool_reallocate)
         {
            auto small = pool::Holder::instance().allocate( buffer::type::binary(), 64);

            auto big = pool::Holder::instance().reallocate( small, 2048);

            EXPECT_TRUE( small != nullptr);
            EXPECT_TRUE( big != nullptr);
            EXPECT_TRUE( small != big);

            pool::Holder::instance().deallocate( big);

         }

         TEST( casual_common_buffer, pool_reallocate__reallocate_again_with_old__throws)
         {
            auto small = pool::Holder::instance().allocate( buffer::type::binary(), 64);
            auto big = pool::Holder::instance().reallocate( small, 2048);

            pool::Holder::instance().deallocate( big);

            EXPECT_THROW({
               pool::Holder::instance().reallocate( small, 2048);
            }, exception::xatmi::invalid::Argument);

         }

         TEST( casual_common_buffer, pool_type)
         {
            const auto type = buffer::type::binary();
            auto buffer = pool::Holder::instance().allocate( type, 64);

            auto&& result = pool::Holder::instance().type( buffer);

            EXPECT_TRUE( type == result);

            // will log warning
            pool::Holder::instance().clear();

         }

         TEST( casual_common_buffer, pool_get_user_size)
         {
            const auto type = buffer::type::binary();
            const std::string info( "test string");

            auto handle = pool::Holder::instance().allocate( type, 128);
            range::copy( info, handle);

            auto holder = buffer::pool::Holder::instance().get( handle, 100);

            EXPECT_TRUE( holder.transport == 100);
            EXPECT_TRUE( holder.payload.memory.size() == 128) << "holder.payload.memory.size(): " << holder.payload.memory.size();
            EXPECT_TRUE( holder.payload.memory.data() == info);

            buffer::pool::Holder::instance().deallocate( handle);
         }

         TEST( casual_common_buffer, message_call)
         {

            platform::binary_type marshal_buffer;

            const std::string info( "test string");
            const auto type = buffer::type::binary();

            // marshal
            {
               auto handle = pool::Holder::instance().allocate( type, 128);
               range::copy( info, handle);

               message::service::call::caller::Request message( buffer::pool::Holder::instance().get( handle, 100));

               EXPECT_TRUE( message.buffer.transport == 100);
               EXPECT_TRUE( message.buffer.payload.memory.size() == 128) << "message.buffer.payload.memory.size(): " << message.buffer.payload.memory.size();
               EXPECT_TRUE( message.buffer.payload.memory.data() == info);

               marshal::output::Binary output( marshal_buffer);
               output << message;

               buffer::pool::Holder::instance().deallocate( handle);

            }

            // unmarshal
            {

               message::service::call::callee::Request message;

               marshal::input::Binary input( marshal_buffer);
               input >> message;

               EXPECT_TRUE( message.buffer.type == type);
               EXPECT_TRUE( message.buffer.memory.size() == 100);
               EXPECT_TRUE( message.buffer.memory.data() == info)  << " message.buffer.memory.data(): " <<  message.buffer.memory.data();
            }
         }

      } // buffer
	} // common
} // casual



