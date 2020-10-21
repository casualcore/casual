//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/serialize/native/binary.h"
#include "common/serialize/native/network.h"
#include "common/serialize/native/complete.h"

#include "common/message/service.h"
#include "common/message/queue.h"


#include "common/transaction/id.h"

#include "casual/platform.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace native
         {
            
            template <typename T>
            struct casual_serialize_native_binary : public ::testing::Test
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
                  IO< binary::create::Reader, binary::create::Writer>,
                  IO< binary::network::create::Reader, binary::network::create::Writer>
            >;

            TYPED_TEST_SUITE(casual_serialize_native_binary, marshal_types);


            TYPED_TEST( casual_serialize_native_binary, basic_io)
            {
               common::unittest::Trace trace;

               using input_type = typename TestFixture::input_type;
               using output_type = typename TestFixture::output_type;

               long someLong = 3;
               std::string someString = "banan";

               auto output = output_type{}();

               output << someLong;
               output << someString;
               
               auto buffer = output.consume();
               auto input = input_type{}( buffer);

               long resultLong;
               input >> resultLong;
               EXPECT_TRUE( resultLong == someLong) << resultLong;

               std::string resultString;
               input >> resultString;
               EXPECT_TRUE( resultString == someString) << resultString;
            }



            TYPED_TEST( casual_serialize_native_binary, binary)
            {
               common::unittest::Trace trace;

               using input_type = typename TestFixture::input_type;
               using output_type = typename TestFixture::output_type;

               auto binaryInput = unittest::random::binary( 3000);

               auto output = output_type{}();
               output << binaryInput;

               auto buffer = output.consume();
               EXPECT_TRUE( buffer.size() == binaryInput.size() + sizeof( binaryInput.size())) <<  buffer.size();

               auto input = input_type{}( buffer);

               std::vector< char> binaryOutput;
               input >> binaryOutput;

               EXPECT_TRUE( binaryInput == binaryOutput);
            }

            TYPED_TEST( casual_serialize_native_binary, io)
            {
               common::unittest::Trace trace;

               using input_type = typename TestFixture::input_type;
               using output_type = typename TestFixture::output_type;

               message::service::Advertise advertise;

               const auto ipc = strong::ipc::id{ uuid::make()};

               advertise.process.ipc = ipc;

               traits::iterable::value_t< decltype( advertise.services.add)> service;

               service.name = "service1";
               advertise.services.add.push_back( service);

               service.name = "service2";
               advertise.services.add.push_back( service);

               service.name = "service3";
               advertise.services.add.push_back( service);


               auto complete = serialize::native::complete( advertise, output_type{});
               message::service::Advertise result;
               serialize::native::complete( complete, result, input_type{});

               EXPECT_TRUE( result.process.ipc == ipc) << result.process.ipc;
               EXPECT_TRUE( result.services.add.size() == 3) << result.services.add.size();

            }


            TYPED_TEST( casual_serialize_native_binary, io_big_size)
            {
               common::unittest::Trace trace;

               using input_type = typename TestFixture::input_type;
               using output_type = typename TestFixture::output_type;

               message::service::Advertise advertise;

               const auto ipc = strong::ipc::id{ uuid::make()};

               advertise.process.ipc = ipc;


               traits::iterable::value_t< decltype( advertise.services.add)> service;

               service.name = "service1";
               advertise.services.add.resize( 10000, service);

               auto complete = serialize::native::complete( advertise, output_type{});

               message::service::Advertise result;

               serialize::native::complete( complete, result, input_type{});

               EXPECT_TRUE( result.process.ipc == ipc) << result.process.ipc;
               EXPECT_TRUE( result.services.add.size() == 10000) << result.services.add.size();

            }

            TYPED_TEST( casual_serialize_native_binary, transaction_id_null)
            {
               common::unittest::Trace trace;

               using input_type = typename TestFixture::input_type;
               using output_type = typename TestFixture::output_type;

               transaction::ID xid_source;
               auto output = output_type{}();

               output << xid_source;

               auto buffer = output.consume();
               auto input = input_type{}( buffer);

               transaction::ID xid_target;

               input >> xid_target;

               EXPECT_TRUE( xid_target.null());
            }

            TYPED_TEST( casual_serialize_native_binary, transaction_id)
            {
               common::unittest::Trace trace;

               using input_type = typename TestFixture::input_type;
               using output_type = typename TestFixture::output_type;

               auto xid_source = transaction::id::create();

               auto output = output_type{}();

               output & xid_source;

               auto buffer = output.consume();
               auto input = input_type{}( buffer);

               transaction::ID xid_target;

               input & xid_target;

               EXPECT_TRUE( ! xid_target.null());
               EXPECT_TRUE( xid_target == xid_source);
            }

            TYPED_TEST( casual_serialize_native_binary, serialization_unit_min)
            {
               common::unittest::Trace trace;

               using input_type = typename TestFixture::input_type;
               using output_type = typename TestFixture::output_type;

               const auto expected = platform::time::serialization::unit::zero();

               auto output = output_type{}();
               output << expected;

               platform::time::serialization::unit target;
               {
                  auto buffer = output.consume();
                  auto input = input_type{}( buffer);
                  input >> target;
               }
               
               EXPECT_TRUE( target == expected) 
                  << CASUAL_NAMED_VALUE( target.count()) 
                  << "\n" << CASUAL_NAMED_VALUE( expected.count());

               EXPECT_TRUE( target == platform::time::serialization::unit::zero());
            }

            TYPED_TEST( casual_serialize_native_binary, time_unit_min)
            {
               common::unittest::Trace trace;

               using input_type = typename TestFixture::input_type;
               using output_type = typename TestFixture::output_type;

               const auto expected = platform::time::unit::zero();

               auto output = output_type{}();
               output << expected;

               platform::time::unit target;
               {
                  auto buffer = output.consume();
                  auto input = input_type{}( buffer);
                  input >> target;
               }
               
               EXPECT_TRUE( target == expected) 
                  << CASUAL_NAMED_VALUE( target.count()) 
                  << "\n" << CASUAL_NAMED_VALUE( expected.count());

               EXPECT_TRUE( target == platform::time::unit::zero());
            }

            TYPED_TEST( casual_serialize_native_binary, time_point_min)
            {
               common::unittest::Trace trace;

               using input_type = typename TestFixture::input_type;
               using output_type = typename TestFixture::output_type;

               const platform::time::point::type expected;

               auto output = output_type{}();
               output << expected;

               platform::time::point::type target;
               {
                  auto buffer = output.consume();
                  auto input = input_type{}( buffer);
                  input >> target;
               }
               
               EXPECT_TRUE( target == expected) 
                  << CASUAL_NAMED_VALUE( target.time_since_epoch().count()) 
                  << "\n" << CASUAL_NAMED_VALUE( expected.time_since_epoch().count());

               EXPECT_TRUE( target.time_since_epoch().count() == 0);
            }

            TYPED_TEST( casual_serialize_native_binary, message_call)
            {
               common::unittest::Trace trace;

               using input_type = typename TestFixture::input_type;
               using output_type = typename TestFixture::output_type;

               const std::string info( "test string");
               const std::string type{ "X_OCTET/binary"};

               auto output = output_type{}();

               // marshal
               {
                  // header
                  {
                     auto& header = service::header::fields();
                     header.clear();

                     header[ "casual.header.test.1"] = "42";
                     header[ "casual.header.test.2"] = "poop";
                  }


                  buffer::Payload payload{ type, 128};
                  algorithm::copy( info, std::begin( payload.memory));

                  EXPECT_TRUE( payload.memory.size() == 128) << " payload.memory.size(): " <<  payload.memory.size();
                  EXPECT_TRUE( payload.memory.data() == info) << "payload.memory.data(): " <<  payload.memory.data();

                  message::service::call::caller::Request message{ buffer::payload::Send{ payload, 100, 100}};
                  message.header = service::header::fields();

                  output << message;

                  // header
                  {
                     service::header::fields().clear();
                     EXPECT_TRUE( service::header::fields().empty());
                  }
               }

               // unmarshal
               {

                  message::service::call::callee::Request message;

                  auto buffer = output.consume();
                  auto input = input_type{}( buffer);
                  input >> message;

                  EXPECT_TRUE( message.buffer.type == type);
                  EXPECT_TRUE( message.buffer.memory.size() == 100) << "message.buffer.memory.size(): " << message.buffer.memory.size();
                  EXPECT_TRUE( message.buffer.memory.data() == info)  << " message.buffer.memory.data(): " <<  message.buffer.memory.data();

                  // header
                  {
                     
                     EXPECT_TRUE( message.header.size() == 2);
                     EXPECT_TRUE( message.header.at< int>( "casual.header.test.1") == 42);
                     EXPECT_TRUE( message.header.at( "casual.header.test.2") == "poop");
                  }

               }

            }

            TYPED_TEST( casual_serialize_native_binary, enqueue_request)
            {
               common::unittest::Trace trace;

               using input_type = typename TestFixture::input_type;
               using output_type = typename TestFixture::output_type;

               common::message::queue::enqueue::Request source;

               {
                  source.process = process::handle();
                  source.message.payload = { 0, 2, 3, 4, 2, 45, 45, 2, 3};
               }

               auto output = output_type{}();
               output & source;

               auto buffer = output.consume();
               auto input = input_type{}( buffer);
               common::message::queue::enqueue::Request target;

               input & target;

               EXPECT_TRUE( source.process == target.process);
               EXPECT_TRUE( source.message.payload == target.message.payload);

            }
         } // native
      } // serialize
   } // common
} // casual



