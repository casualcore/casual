//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <common/unittest.h>

#include "common/buffer/pool.h"

#include "common/message/service.h"
#include "common/serialize/native/binary.h"


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
           */

         TEST( common_buffer, default_buffer)
         {
            common::unittest::Trace trace;

            pool::implementation::default_buffer buffer;

            EXPECT_THROW(
               buffer.get( {});
            ,std::system_error);
         }

         TEST( common_buffer, pool_allocate)
         {
            common::unittest::Trace trace;

            auto buffer = pool::holder().allocate( buffer::type::binary, 1024);

            ASSERT_TRUE( buffer);

            pool::holder().deallocate( buffer);

         }

         TEST( common_buffer, pool_adopt)
         {
            common::unittest::Trace trace;

            auto buffer = pool::holder().adopt( Payload{ buffer::type::binary, 1024});

            ASSERT_TRUE( buffer);

            pool::holder().clear();
         }

         TEST( common_buffer, pool_adopt__deallocate__expect__inbound_still_valid)
         {
            common::unittest::Trace trace;

            auto inbound = pool::holder().adopt( Payload{ buffer::type::binary, 1024});

            pool::holder().deallocate( inbound);

            EXPECT_NO_THROW({
               auto send = pool::holder().get( inbound);
               EXPECT_TRUE( send.reserved() == 1024);
            });

            pool::holder().clear();
         }

         TEST( common_buffer, pool_adopt__clear__expect__inbound_deallocated)
         {
            common::unittest::Trace trace;

            auto inbound = pool::holder().adopt( Payload{ buffer::type::binary, 1024});

            pool::holder().clear();

            EXPECT_CODE({
               pool::holder().get( inbound);
            }, code::xatmi::argument);

         }

         TEST( common_buffer, pool_allocate_non_existing__throws)
         {
            common::unittest::Trace trace;

            EXPECT_CODE({
               pool::holder().allocate( "non-existing", "non-existing", 1024);
            }, code::xatmi::buffer_input);
         }


         TEST( common_buffer, pool_reallocate)
         {
            common::unittest::Trace trace;

            auto small = pool::holder().allocate( buffer::type::binary, 64);

            auto big = pool::holder().reallocate( small, 2048);

            EXPECT_TRUE( small);
            EXPECT_TRUE( big);
            EXPECT_TRUE( small != big);

            pool::holder().deallocate( big);

         }

         TEST( common_buffer, pool_reallocate__reallocate_again_with_old__throws)
         {
            common::unittest::Trace trace;

            auto small = pool::holder().allocate( buffer::type::binary, 64);
            auto big = pool::holder().reallocate( small, 2048);

            pool::holder().deallocate( big);

            EXPECT_CODE({
               pool::holder().reallocate( small, 2048);
            }, code::xatmi::argument);

         }

         TEST( common_buffer, pool_type)
         {
            common::unittest::Trace trace;

            const auto type = buffer::type::binary;
            auto buffer = pool::holder().allocate( type, 64);

            auto&& result = pool::holder().type( buffer);

            EXPECT_TRUE( type == result);

            // will log warning
            pool::holder().clear();

         }

         TEST( common_buffer, pool_get_user_size)
         {
            common::unittest::Trace trace;

            const auto type = buffer::type::binary;
            const std::string info( "test string");

            auto handle = pool::holder().allocate( type, 128);
            algorithm::copy( info, handle.underlying());

            auto holder = buffer::pool::holder().get( handle, 100);

            EXPECT_TRUE( holder.transport() == 100);
            EXPECT_TRUE( holder.payload().data.size() == 128) << "holder.payload.memory.size(): " << holder.payload().data.size();
            EXPECT_TRUE( holder.payload().data.data() == info);

            buffer::pool::holder().deallocate( handle);
         }

         TEST( common_buffer, message_call)
         {
            common::unittest::Trace trace;

            platform::binary::type marshal_buffer;

            const std::string info( "test string");
            const auto type = buffer::type::binary;

            // marshal
            {
               auto handle = pool::holder().allocate( type, 128);
               algorithm::copy( info, handle.underlying());

               message::service::call::caller::Request message( buffer::pool::holder().get( handle, 100));

               EXPECT_TRUE( message.buffer.transport() == 100);
               EXPECT_TRUE( message.buffer.payload().data.size() == 128) << "message.buffer.payload.memory.size(): " << message.buffer.payload().data.size();
               EXPECT_TRUE( message.buffer.payload().data.data() == info);

               serialize::native::binary::Writer output;
               output << message;
               marshal_buffer = output.consume();
               
               buffer::pool::holder().deallocate( handle);
            }

            // unmarshal
            {

               message::service::call::callee::Request message;

               serialize::native::binary::Reader input( marshal_buffer);
               input >> message;

               EXPECT_TRUE( message.buffer.type == type);
               EXPECT_TRUE( message.buffer.data.size() == 100);
               EXPECT_TRUE( message.buffer.data.data() == info)  << " message.buffer.data.data(): " <<  message.buffer.data.data();
            }
         }

         TEST( common_buffer_type, serialize__payload_Send)
         {
            common::unittest::Trace trace;
            
            platform::binary::type buffer;
            
            buffer::Payload payload;
            {
               payload.type = "foo";
               payload.data = unittest::random::binary( 256);
            }
            {
               serialize::native::binary::Writer output;
               output << buffer::payload::Send{ payload};
               buffer = output.consume();
            }

            {
               buffer::Payload serialized;
               serialize::native::binary::Reader input( buffer);
               input >> serialized;

               EXPECT_TRUE( payload.type == serialized.type);
               EXPECT_TRUE( algorithm::equal( payload.data, serialized.data)); 
            }
         }

      } // buffer
   } // common
} // casual



