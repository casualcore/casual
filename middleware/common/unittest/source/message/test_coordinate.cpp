//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/unittest.h"

#include "common/log/stream.h"

#include "common/message/coordinate.h"
#include "common/message/gateway.h"
#include "common/communication/ipc.h"


namespace casual
{
   namespace common::message
   {
      namespace local
      {
         namespace
         {
            using Coordinate = message::coordinate::fan::Out< message::gateway::domain::discover::Reply, strong::process::id>;
         } // <unnamed>
      } // local


      TEST( common_message_coordinate, instantiation)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            local::Coordinate coordinate;
         });
      }

      TEST( common_message_coordinate, add_empty_pending__expect_direct_invoke)
      {
         common::unittest::Trace trace;

         local::Coordinate coordinate;
         bool invoked = false;

         coordinate( {}, [&invoked]( auto received, auto failed)
         {
            invoked = true;
         });

         EXPECT_TRUE( invoked);
      }

      TEST( common_message_coordinate, add_1_pending__expect_no_direct_invoke)
      {
         common::unittest::Trace trace;


         local::Coordinate coordinate;
         bool invoked = false;

         auto correlation = uuid::make();

         coordinate( { { correlation, process::id()}}, [&invoked]( auto received, auto failed)
         {
            invoked = true;
         });

         EXPECT_TRUE( ! invoked);
      }

      TEST( common_message_coordinate, add_1_pending__add_message___expect_invoke)
      {
         common::unittest::Trace trace;


         local::Coordinate coordinate;
         bool invoked = false;

         auto correlation = uuid::make();

         coordinate( { { correlation, process::id()}}, [&invoked]( auto received, auto failed)
         {
            invoked = true;
         });

         {
            message::gateway::domain::discover::Reply message{ process::handle()};
            message.correlation = correlation;
            coordinate( message);
         }

         EXPECT_TRUE( invoked);
      }


      TEST( common_message_coordinate, add_10_pending__add_10_message___expect_invoke)
      {
         common::unittest::Trace trace;

         bool invoked = false;

         // fill the messages
         const auto origin = algorithm::generate_n< 10>( []( auto index)
         {
            message::gateway::domain::discover::Reply message{ process::Handle{ strong::process::id( process::id().value() + index), communication::ipc::inbound::ipc()}};
            message.correlation = uuid::make();
            return message;
         });

         local::Coordinate coordinate;

         // fill with fan out entries.
         coordinate(
            algorithm::transform( origin, []( auto& message)
            {
               return local::Coordinate::Pending{ message.correlation, message.process.pid};
            }),
            [&invoked, &origin]( auto received, auto failed)
            {
               EXPECT_TRUE( algorithm::equal( origin, received, []( auto& l, auto& r){ return l.correlation == r.correlation;}));
               invoked = true;
            });


         EXPECT_TRUE( ! coordinate.empty());

         // accumulate all messages
         algorithm::for_each( origin, std::ref( coordinate));
         

         EXPECT_TRUE( invoked);
         EXPECT_TRUE( coordinate.empty()) << CASUAL_NAMED_VALUE( coordinate);
      }

      TEST( common_message_coordinate, add_10_pending__add_9_message___expect_not_invoke)
      {
         common::unittest::Trace trace;

         bool invoked = false;

         // fill the messages
         const auto origin = algorithm::generate_n< 10>( []()
         {
            message::gateway::domain::discover::Reply message{ process::handle()};
            message.correlation = uuid::make();
            return message;
         });

         local::Coordinate coordinate;

         // fill with fan out entries.
         coordinate(
            algorithm::transform( origin, []( auto& message)
            {
               return local::Coordinate::Pending{ message.correlation, message.process.pid};
            }),
            [&invoked]( auto received, auto failed)
            {
               invoked = true;
            });


         // accumulate first 9 messages
         algorithm::for_each( range::make( std::begin( origin), 9), std::ref( coordinate));
         
         EXPECT_TRUE( ! invoked);
         EXPECT_TRUE( ! coordinate.empty()) << CASUAL_NAMED_VALUE( coordinate);

         coordinate.failed( process::id());         

         EXPECT_TRUE( invoked);
         EXPECT_TRUE( coordinate.empty()) << CASUAL_NAMED_VALUE( coordinate);
      }

      TEST( common_message_coordinate, add_10_pending__same_pid__failed_pid___expect_invoked)
      {
         common::unittest::Trace trace;

         bool invoked = false;

         // fill the messages
         const auto origin = algorithm::generate_n< 10>( []()
         {
            message::gateway::domain::discover::Reply message{ process::handle()};
            message.correlation = uuid::make();
            return message;
         });

         local::Coordinate coordinate;

         // fill with fan out entries.
         coordinate(
            algorithm::transform( origin, []( auto& message)
            {
               return local::Coordinate::Pending{ message.correlation, message.process.pid};
            }),
            [&invoked]( auto received, auto failed)
            {
               invoked = true;
            });

         coordinate.failed( process::id());         

         EXPECT_TRUE( invoked);
         EXPECT_TRUE( coordinate.empty()) << CASUAL_NAMED_VALUE( coordinate);
      }

   } // common::message
} // casual
