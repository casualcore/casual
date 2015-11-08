//!
//! casual_isolatedunittest_archive.cpp
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/marshal/binary.h"
#include "common/marshal/network.h"

#include "common/message/service.h"
#include "common/message/queue.h"


#include "common/transaction/id.h"

#include "common/platform.h"

namespace casual
{
   namespace common
   {
      namespace marshal
      {

         template <typename T>
         struct casual_common_marshal : public ::testing::Test
         {
            using input_type = typename T::input_type;
            using output_type = typename T::output_type;
         };

         template< typename I, typename O>
         struct IO
         {
            using input_type = I;
            using output_type = O;
         };


         using marshal_types = ::testing::Types<
               IO< binary::create::Input, binary::create::Output>,
               IO< binary::network::create::Input, binary::network::create::Output>
         >;

         TYPED_TEST_CASE(casual_common_marshal, marshal_types);


         TYPED_TEST( casual_common_marshal, basic_io)
         {
            using input_type = typename TestFixture::input_type;
            using output_type = typename TestFixture::output_type;

            long someLong = 3;
            std::string someString = "banan";

            platform::binary_type buffer;
            auto output = output_type{}( buffer);

            output << someLong;

            EXPECT_TRUE( buffer.size() == sizeof( long)) <<  buffer.size();

            output << someString;

            auto input = input_type{}( buffer);

            long resultLong;
            input >> resultLong;
            EXPECT_TRUE( resultLong == someLong) << resultLong;

            std::string resultString;
            input >> resultString;
            EXPECT_TRUE( resultString == someString) << resultString;
         }



         TYPED_TEST( casual_common_marshal, binary)
         {
            using input_type = typename TestFixture::input_type;
            using output_type = typename TestFixture::output_type;

            std::vector< char> binaryInput;

            for( int index = 0; index < 3000; ++index)
            {
               binaryInput.push_back( static_cast< char>( index));

            }

            platform::binary_type buffer;
            auto output = output_type{}( buffer);
            output << binaryInput;

            EXPECT_TRUE( buffer.size() == binaryInput.size() + sizeof( binaryInput.size())) <<  buffer.size();

            auto input = input_type{}( buffer);

            std::vector< char> binaryOutput;
            input >> binaryOutput;

            EXPECT_TRUE( binaryInput == binaryOutput);
         }

         TYPED_TEST( casual_common_marshal, io)
         {
            using input_type = typename TestFixture::input_type;
            using output_type = typename TestFixture::output_type;

            message::service::Advertise serverConnect;

            serverConnect.process.queue = 666;
            serverConnect.serverPath = "/bla/bla/bla/sever";

            message::Service service;

            service.name = "service1";
            serverConnect.services.push_back( service);

            service.name = "service2";
            serverConnect.services.push_back( service);

            service.name = "service3";
            serverConnect.services.push_back( service);


            ipc::message::Complete complete = marshal::complete( serverConnect, output_type{});

            message::service::Advertise result;

            marshal::complete( complete, result, input_type{});

            EXPECT_TRUE( result.process.queue == 666) << result.process.queue;
            EXPECT_TRUE( result.serverPath == "/bla/bla/bla/sever") << result.serverPath;
            EXPECT_TRUE( result.services.size() == 3) << result.services.size();

         }


         TYPED_TEST( casual_common_marshal, io_big_size)
         {
            using input_type = typename TestFixture::input_type;
            using output_type = typename TestFixture::output_type;

            message::service::Advertise serverConnect;

            serverConnect.process.queue = 666;
            serverConnect.serverPath = "/bla/bla/bla/sever";


            message::Service service;
            service.name = "service1";
            serverConnect.services.resize( 10000, service);

            ipc::message::Complete complete = marshal::complete( serverConnect, output_type{});

            message::service::Advertise result;

            marshal::complete( complete, result, input_type{});

            EXPECT_TRUE( result.process.queue == 666) << result.process.queue;
            EXPECT_TRUE( result.serverPath == "/bla/bla/bla/sever") << result.serverPath;
            EXPECT_TRUE( result.services.size() == 10000) << result.services.size();

         }

         TYPED_TEST( casual_common_marshal, transaction_id_null)
         {
            using input_type = typename TestFixture::input_type;
            using output_type = typename TestFixture::output_type;

            transaction::ID xid_source;

            platform::binary_type buffer;
            auto output = output_type{}( buffer);

            output << xid_source;

            auto input = input_type{}( buffer);

            transaction::ID xid_target;

            input >> xid_target;

            EXPECT_TRUE( xid_target.null());
         }

         TYPED_TEST( casual_common_marshal, transaction_id)
         {
            using input_type = typename TestFixture::input_type;
            using output_type = typename TestFixture::output_type;

            transaction::ID xid_source = transaction::ID::create();

            platform::binary_type buffer;
            auto output = output_type{}( buffer);

            output & xid_source;

            auto input = input_type{}( buffer);

            transaction::ID xid_target;

            input & xid_target;

            EXPECT_TRUE( ! xid_target.null());
            EXPECT_TRUE( xid_target == xid_source);

         }


         TYPED_TEST( casual_common_marshal, message_call)
         {
            using input_type = typename TestFixture::input_type;
            using output_type = typename TestFixture::output_type;

            platform::binary_type buffer;

            const std::string info( "test string");
            const buffer::Type type{ "X_OCTET", "binary"};

            // marshal
            {

               buffer::Payload payload{ type, 128};
               range::copy( info, std::begin( payload.memory));

               EXPECT_TRUE( payload.memory.size() == 128) << " payload.memory.size(): " <<  payload.memory.size();
               EXPECT_TRUE( payload.memory.data() == info) << "payload.memory.data(): " <<  payload.memory.data();

               message::service::call::caller::Request message{ buffer::payload::Send{ payload, 100, 100}};

               auto output = output_type{}( buffer);
               output << message;

            }

            // unmarshal
            {

               message::service::call::callee::Request message;

               auto input = input_type{}( buffer);
               input >> message;

               EXPECT_TRUE( message.buffer.type == type);
               EXPECT_TRUE( message.buffer.memory.size() == 100);
               EXPECT_TRUE( message.buffer.memory.data() == info)  << " message.buffer.memory.data(): " <<  message.buffer.memory.data();

            }

         }

         TYPED_TEST( casual_common_marshal, enqueue_request)
         {
            using input_type = typename TestFixture::input_type;
            using output_type = typename TestFixture::output_type;

            platform::binary_type buffer;

            common::message::queue::enqueue::Request source;

            {
               source.process = process::handle();
               source.message.payload = { 0, 2, 3, 4, 2, 45, 45, 2, 3};
            }

            auto output = output_type{}( buffer);
            output & source;

            auto input = input_type{}( buffer);
            common::message::queue::enqueue::Request target;

            input & target;

            EXPECT_TRUE( source.process == target.process);
            EXPECT_TRUE( source.message.payload == target.message.payload);

         }
      }

   }
}



