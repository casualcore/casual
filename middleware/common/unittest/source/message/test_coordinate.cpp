//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/unittest.h"

#include "common/log/stream.h"

#include "common/message/coordinate.h"
#include "common/communication/ipc.h"


namespace casual
{
   namespace common::message
   {
      namespace local
      {
         namespace
         {
            using Request = message::basic_request< message::Type::domain_discovery_request>;
            using Reply = message::basic_request< message::Type::domain_discovery_reply>;

            using Coordinate = message::coordinate::fan::Out< Reply, strong::process::id>;
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

         auto correlation = strong::correlation::id::emplace( uuid::make());

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

         auto correlation = strong::correlation::id::emplace( uuid::make());

         coordinate( { { correlation, process::id()}}, [&invoked]( auto received, auto failed)
         {
            invoked = true;
         });

         {
            local::Reply message{ process::handle()};
            message.correlation = correlation;
            coordinate( message);
         }

         EXPECT_TRUE( invoked) << trace.compose( "coordinate: ", coordinate);
      }


      TEST( common_message_coordinate, add_10_pending__add_10_message___expect_invoke)
      {
         common::unittest::Trace trace;

         bool invoked = false;

         // fill the messages
         const auto origin = algorithm::generate_n< 10>( []( auto index)
         {
            local::Reply message{ process::Handle{ strong::process::id( process::id().value() + index), communication::ipc::inbound::ipc()}};
            message.correlation = strong::correlation::id::emplace( uuid::make());
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
         EXPECT_TRUE( coordinate.empty()) << trace.compose( "coordinate: ", coordinate);
      }

      TEST( common_message_coordinate, add_10_pending__add_9_message___expect_not_invoke)
      {
         common::unittest::Trace trace;

         bool invoked = false;

         // fill the messages
         const auto origin = algorithm::generate_n< 10>( []()
         {
            local::Reply message{ process::handle()};
            message.correlation = strong::correlation::id::emplace( uuid::make());
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
         EXPECT_TRUE( ! coordinate.empty()) << trace.compose( "coordinate: ", coordinate);

         coordinate.failed( process::id());         

         EXPECT_TRUE( invoked);
         EXPECT_TRUE( coordinate.empty()) << trace.compose( "coordinate: ", coordinate);
      }

      TEST( common_message_coordinate, add_10_pending__same_pid__failed_pid___expect_invoked)
      {
         common::unittest::Trace trace;

         bool invoked = false;

         // fill the messages
         const auto origin = algorithm::generate_n< 10>( []()
         {
            local::Reply message{ process::handle()};
            message.correlation = strong::correlation::id::emplace( uuid::make());
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
         EXPECT_TRUE( coordinate.empty()) << trace.compose( "coordinate: ", coordinate);
      }

      TEST( common_message_coordinate, add_10_pending__add_9__received_message_1_failed_perform_purge___expect_invoke)
      {
         common::unittest::Trace trace;

         bool invoked = false;

         // fill the messages
         auto origin = algorithm::generate_n< 8>( []()
         {
            local::Reply message{ process::handle()};
            message.correlation = strong::correlation::id::emplace( uuid::make());
            return message;
         });

         // 'other' process information
         constexpr auto other_pid = 19700101;
         const std::vector< decltype( uuid::make()) > other_corrid{ uuid::make(), uuid::make()};
         const Process::Handle other_process{ static_cast< strong::process::id>( other_pid)};

         {
            // add 'other' process
            local::Reply message{ other_process};
            message.correlation = strong::correlation::id::emplace( other_corrid.at( 0));
            origin.push_back( message);
         }

         {
            // and again
            local::Reply message{ other_process};
            message.correlation = strong::correlation::id::emplace( other_corrid.at( 1));
            origin.push_back( message);
         }

         local::Coordinate coordinate;

         // fill with fan out entries.
         coordinate(
            algorithm::transform( origin, []( auto& message)
            {
               return local::Coordinate::Pending{ message.correlation, message.process.pid};
            }),
            [&invoked, &other_process]( auto received, auto failed)
            {
               {
                  // all current process pendings is marked as failed
                  decltype( failed) pendings;
                  algorithm::copy_if( failed, std::back_inserter( pendings), [ pid = process::id()]( auto pending)
                  {
                     return pid == pending.id;
                  });
                  ASSERT_TRUE( ! pendings.empty());
                  EXPECT_TRUE(
                     algorithm::all_of( pendings, []( auto pending){
                        return pending.state == local::Coordinate::Pending::State::failed;
                     })
                  );
               }
               {
                  // all 'other' process pendings is still marked as received
                  decltype( failed) pendings;
                  algorithm::copy_if( failed, std::back_inserter( pendings), [ &other_process]( auto pending)
                  {
                     return other_process.pid == pending.id;
                  });
                  ASSERT_TRUE( ! pendings.empty());
                  EXPECT_TRUE(
                     algorithm::all_of( pendings, []( auto pending){
                        return pending.state == local::Coordinate::Pending::State::received;
                     })
                  );
               }
               invoked = true;
            });


         // accumulate first 7 messages with current pid
         algorithm::for_each( range::make( std::begin( origin), 7), std::ref( coordinate));

         {
            // first reply from 'other' process
            local::Reply message{ other_process};
            message.correlation = strong::correlation::id::emplace( other_corrid.at( 0));
            coordinate( message);
         }

         EXPECT_TRUE( ! invoked);
         EXPECT_TRUE( ! coordinate.empty()) << trace.compose( "coordinate: ", coordinate);

         // purge all from coordinate with current pid
         coordinate.failed( process::id());

         // still one message from 'other' process to be received
         EXPECT_TRUE( ! invoked);
         EXPECT_TRUE( ! coordinate.empty()) << trace.compose( "coordinate: ", coordinate);

         {
            // last message from 'other' process beeing coordinated
            local::Reply message{ other_process};
            message.correlation = strong::correlation::id::emplace( other_corrid.at( 1));
            coordinate( message);
         }

         // everything handled
         EXPECT_TRUE( invoked);
         EXPECT_TRUE( coordinate.empty()) << trace.compose( "coordinate: ", coordinate);
      }

   } // common::message
} // casual
