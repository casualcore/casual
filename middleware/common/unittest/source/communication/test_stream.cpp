//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/communication/stream.h"
#include "common/message/type.h"
#include "common/compare.h"
#include "common/message/dispatch.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace stream
         {
            namespace local
            {
               namespace
               {
                  using base_message = common::message::basic_message< common::message::Type::unittest_message>;
                  struct Message : base_message, Compare< Message>
                  {
                     long m_long{};
                     short m_short{};
                     std::string m_string;
                     platform::binary::type m_binary;
                     
                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        base_message::serialize( archive);
                        CASUAL_SERIALIZE( m_long);
                        CASUAL_SERIALIZE( m_short);
                        CASUAL_SERIALIZE( m_string);
                        CASUAL_SERIALIZE( m_binary);
                     })

                     auto tie() const { return std::tie( m_long, m_short, m_string, m_binary);}
                  };

                  auto message()
                  {
                     local::Message result;
                     unittest::random::set( result.m_long);
                     unittest::random::set( result.m_short);
                     result.m_string = unittest::random::string( 128);
                     result.m_binary = unittest::random::binary( 1024);
                     return result;
                  }

               } // <unnamed>
            } // local
   
            TEST( common_communication_stream, serialize__message)
            {
               unittest::Trace trace;

               auto origin = local::message();

               std::stringstream stream;

               stream::outbound::Device out{ stream};
               device::blocking::send( out, origin);
               
               local::Message message;

               communication::stream::inbound::Device in{ stream};
               device::blocking::receive( in, message);

               EXPECT_TRUE( origin == message) << CASUAL_NAMED_VALUE( message);
            }

            TEST( common_communication_stream, send_message__dispatch)
            {
               common::unittest::Trace trace;
               
               std::stringstream stream;
               
               auto origin = local::message();

               stream::outbound::Device out{ stream};
               device::blocking::send( out, origin);

               communication::stream::inbound::Device in{ stream};

               auto handler = common::message::dispatch::handler( in,
                  [&origin]( const local::Message& message)
                  {
                     EXPECT_TRUE( origin == message);
                  });

               EXPECT_TRUE( handler( device::blocking::next( in)));
            }

            TEST( common_communication_stream, send_10_messages__dispatch)
            {
               common::unittest::Trace trace;
               
               std::stringstream stream;
               auto origin = local::message();

               stream::outbound::Device out{ stream};
               
               long count = 0;

               for( auto index = 0; index < 10; ++index)
               {
                  device::blocking::send( out, origin);
                  ++count;
               }

               long dispatch_count = 0;

               communication::stream::inbound::Device in{ stream};

               auto handler = common::message::dispatch::handler( in,
                  [&]( const local::Message& message)
                  {
                     ++dispatch_count;
                     EXPECT_TRUE( origin == message);
                  });

               while( dispatch_count != count && handler( device::blocking::next( in)))
                  ; // no-op

               EXPECT_TRUE( count == dispatch_count) << "count: " << count << ", dispatch_count: " << dispatch_count;
            }
            
         }

      } // communication
   } // common
} // casual