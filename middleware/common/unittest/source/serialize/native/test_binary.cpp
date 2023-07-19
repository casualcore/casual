//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/serialize/native/binary.h"
#include "common/serialize/native/network.h"
#include "common/serialize/native/complete.h"
#include "common/serialize/binary.h"
#include "common/communication/ipc/message.h"
#include "common/communication/tcp/message.h"


#include "common/message/service.h"


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
               using complete_type = typename T::complete_type;
               using input_type = typename T::input_type;
               using output_type = typename T::output_type;
            };

            template< typename Complete>
            struct IO
            {
               using complete_type = Complete;
               using serialization = typename serialize::native::customization::point< complete_type>;
               using output_type = typename serialization::writer;
               using input_type = typename serialization::reader;
            };


            using marshal_types = ::testing::Types<
               IO< communication::ipc::message::Complete>,
               IO< communication::tcp::message::Complete>
            >;

            TYPED_TEST_SUITE(casual_serialize_native_binary, marshal_types);


            TYPED_TEST( casual_serialize_native_binary, basic_io)
            {
               common::unittest::Trace trace;

               using input_type = typename TestFixture::input_type;
               using output_type = typename TestFixture::output_type;

               long someLong = 3;
               std::string someString = "banana";

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

               using complete_type = typename TestFixture::complete_type;

               message::service::Advertise advertise;

               const auto ipc = strong::ipc::id{ uuid::make()};

               advertise.process.ipc = ipc;

               std::ranges::range_value_t< decltype( advertise.services.add)> service;

               service.name = "service1";
               advertise.services.add.push_back( service);

               service.name = "service2";
               advertise.services.add.push_back( service);

               service.name = "service3";
               advertise.services.add.push_back( service);


               auto complete = serialize::native::complete< complete_type>( advertise);
               message::service::Advertise result;
               serialize::native::complete( complete, result);

               EXPECT_TRUE( result.process.ipc == ipc) << result.process.ipc;
               EXPECT_TRUE( result.services.add.size() == 3) << result.services.add.size();

            }


            TYPED_TEST( casual_serialize_native_binary, io_big_size)
            {
               common::unittest::Trace trace;

               using complete_type = typename TestFixture::complete_type;

               auto origin = unittest::Message{ 10000};
               origin.correlation = strong::correlation::id::emplace( uuid::make());

               unittest::Message result;

               serialize::native::complete( serialize::native::complete< complete_type>( origin), result);

               EXPECT_TRUE( origin == result) << CASUAL_NAMED_VALUE( origin) << '\n' << CASUAL_NAMED_VALUE( result);
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

               output << xid_source;

               auto buffer = output.consume();
               auto input = input_type{}( buffer);

               transaction::ID xid_target;

               input >> xid_target;

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
                  algorithm::copy( info, std::begin( payload.data));

                  EXPECT_TRUE( payload.data.size() == 128) << " payload.data.size(): " <<  payload.data.size();
                  EXPECT_TRUE( payload.data.data() == info) << "payload.data.data(): " <<  payload.data.data();

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
                  EXPECT_TRUE( message.buffer.data.size() == 100) << "message.buffer.data.size(): " << message.buffer.data.size();
                  EXPECT_TRUE( message.buffer.data.data() == info)  << " message.buffer.data.data(): " <<  message.buffer.data.data();

                  // header
                  {
                     
                     EXPECT_TRUE( message.header.size() == 2);
                     EXPECT_TRUE( message.header.at< int>( "casual.header.test.1") == 42);
                     EXPECT_TRUE( message.header.at( "casual.header.test.2") == "poop");
                  }

               }

            }

            namespace local
            {
               namespace
               {
                  namespace only::host
                  {
                     struct Integrals : common::Compare< Integrals>
                     {
                        Integrals() = default;
                        Integrals( std::int8_t int8, std::int16_t int16, std::int32_t int32, std::int64_t int64, std::uint8_t uint8, std::uint16_t uint16, std::uint32_t uint32, std::uint64_t uint64) 
                           : int8{ int8}, int16{ int16}, int32{ int32}, int64{ int64}, uint8{ uint8}, uint16{ uint16}, uint32{ uint32}, uint64{ uint64} {}

                        std::int8_t int8{};
                        std::int16_t int16{};
                        std::int32_t int32{};
                        std::int64_t int64{};
                        std::uint8_t uint8{};
                        std::uint16_t uint16{};
                        std::uint32_t uint32{};
                        std::uint64_t uint64{};

                        auto tie() const noexcept { return std::tie( int8, int16, int32, int64, uint8, uint16, uint32, uint64);}

                        CASUAL_CONST_CORRECT_SERIALIZE(
                           CASUAL_SERIALIZE( int8);
                           CASUAL_SERIALIZE( int16);
                           CASUAL_SERIALIZE( int32);
                           CASUAL_SERIALIZE( int64);
                           CASUAL_SERIALIZE( uint8);
                           CASUAL_SERIALIZE( uint16);
                           CASUAL_SERIALIZE( uint32);
                           CASUAL_SERIALIZE( uint64);
                        ) 
                     };

                  } // only::host
               } // <unnamed>
            } // local

            TEST( casual_serialize_native_binary_only_host, oddball_integrals)
            {
               unittest::Trace trace;

               const auto origin = local::only::host::Integrals{ 1, 2, 4, 5, 6, 7, 8, 9};

               auto buffer = [ &origin]()
               {
                  auto archive = serialize::native::binary::writer();
                  archive << origin;
                  return archive.consume();
               }();

               {
                  auto value = local::only::host::Integrals{};
                  EXPECT_TRUE( origin != value);
                  
                  auto archive = serialize::native::binary::reader( buffer);
                  archive >> value;
                  
                  EXPECT_TRUE( origin == value);
               }
            };

            TEST( casual_serialize_native_binary_only_host, archive_oddball_integrals)
            {
               unittest::Trace trace;

               const auto origin = local::only::host::Integrals{ 1, 2, 4, 5, 6, 7, 8, 9};

               auto buffer = [ &origin]()
               {
                  auto archive = serialize::binary::writer();
                  archive << origin;
                  platform::binary::type buffer;
                  archive.consume( buffer);
                  return buffer;
               }();

               {
                  auto value = local::only::host::Integrals{};
                  EXPECT_TRUE( origin != value);
                  
                  auto archive = serialize::binary::reader( buffer);
                  archive >> value;
                  
                  EXPECT_TRUE( origin == value);
               }
            };

         } // native
      } // serialize
   } // common
} // casual



