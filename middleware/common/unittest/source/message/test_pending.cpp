//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/message/pending.h"
#include "common/message/service.h"
#include "common/process.h"

#include "common/serialize/native/binary.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace local
         {
            namespace
            {
               auto serialize = []( auto&& message)
               {
                  auto output = serialize::native::binary::writer();
                  output << message;
                  auto buffer = output.consume();

                  std::decay_t< decltype( message)> result;
                  auto input = serialize::native::binary::create::Reader{}( buffer);
                  input >> result;

                  return result;
               };

               auto equal = []( const pending::Message& lhs, const pending::Message& rhs)
               {
                  auto tie = []( const pending::Message& m)
                  { 
                     return std::tie( m.destinations, m.complete.type, m.complete.correlation, m.complete.payload);
                  };

                  return tie( lhs) == tie( rhs);

               };
            } // <unnamed>
         } // local

         TEST( common_message_pending, default_constructed)
         {
            pending::Message origin;

            auto result = local::serialize( origin);

            EXPECT_TRUE( local::equal( origin, result));
         }

         TEST( common_message_pending, small_size)
         {
            pending::Message origin{ []()
            {
               message::service::lookup::Request message;

               message.process = common::process::handle();
               message.context = decltype( message.context)::forward;
               message.requested = "foo";

               return message;
            }(), common::process::handle()};

            auto result = local::serialize( origin);

            EXPECT_TRUE( local::equal( origin, result));
      }

         
         
      } // message
   } // common
} // casual