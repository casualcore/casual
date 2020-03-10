//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/unittest.h"

#include "common/log/stream.h"

#include "common/message/coordinate.h"
#include "common/message/gateway.h"


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
               struct Policy
               {
                  using message_type = gateway::domain::discover::Reply;

                  template< typename M, typename R>
                  void operator() ( M&& request, R&& reply)
                  {
                     correlation = request.correlation;
                  }

                  Uuid correlation;

                  CASUAL_LOG_SERIALIZE(
                  {
                     CASUAL_SERIALIZE( correlation);
                  })
               };

               using Coordinate = message::Coordinate< Policy>;

               struct Send
               {
                  template< typename M>
                  void operator() ( strong::ipc::id queue, M&& message)
                  {
                     sent = true;
                  }
                  bool sent = false;
               };

            } // <unnamed>
         } // local


         TEST( common_message_coordinate, instantiation)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW({
               local::Coordinate coordinate;
            });
         }

         TEST( common_message_coordinate, send__expect_no_coordination)
         {
            common::unittest::Trace trace;

            auto correlation = uuid::make();

            local::Coordinate coordinate;
            local::Send send;
            coordinate.add( correlation, strong::ipc::id{ uuid::make()}, send, std::vector< common::process::Handle>{ { strong::process::id{ 10}, strong::ipc::id{ uuid::make()}}});

            EXPECT_TRUE( send.sent == false);
         }

         TEST( common_message_coordinate, send__accumulate__expect_coordination)
         {
            common::unittest::Trace trace;

            local::Coordinate coordinate;
            local::Send send;

            const auto correlation = uuid::make();
            const auto ipc = strong::ipc::id{ uuid::make()};

            {
               coordinate.add( correlation, strong::ipc::id{ uuid::make()}, send, std::vector< common::process::Handle>{ { strong::process::id{ 42}, ipc}});
            }

            {
               gateway::domain::discover::Reply reply;
               reply.process.pid = strong::process::id{ 42};
               reply.process.ipc = ipc;
               reply.correlation = correlation;
               coordinate.accumulate( reply, send);
            }

            EXPECT_TRUE( send.sent == true) << CASUAL_NAMED_VALUE( coordinate); 
         }

         TEST( common_message_coordinate, send_2_destination__accumulate_1__expect_no_coordination)
         {
            common::unittest::Trace trace;

            local::Coordinate coordinate;
            local::Send send;

            auto correlation = uuid::make();
            const auto ipc_1 = strong::ipc::id{ uuid::make()};
            const auto ipc_2 = strong::ipc::id{ uuid::make()};

            {
               coordinate.add( correlation, strong::ipc::id{ uuid::make()}, send, std::vector< common::process::Handle>{ 
                  {  strong::process::id{ 42}, ipc_1}, {  strong::process::id{ 77}, ipc_2}});
            }

            {
               gateway::domain::discover::Reply reply;
               reply.process.pid = strong::process::id{ 42};
               reply.process.ipc = ipc_1;
               reply.correlation = correlation;
               coordinate.accumulate( reply, send);
            }

            EXPECT_TRUE( send.sent == false);
            EXPECT_TRUE( coordinate.size() == 1);
         }

         TEST( common_message_coordinate, send_2_destination__accumulate_2__expect_coordination)
         {
            common::unittest::Trace trace;

            local::Coordinate coordinate;
            local::Send send;

            auto correlation = uuid::make();

            const auto ipc_1 = strong::ipc::id{ uuid::make()};
            const auto ipc_2 = strong::ipc::id{ uuid::make()};

            {
               coordinate.add( correlation, strong::ipc::id{ uuid::make()}, send,
                  std::vector< common::process::Handle>{ 
                     {  strong::process::id{ 42}, ipc_1}, 
                     {  strong::process::id{ 77}, ipc_2}});
            }

            {
               gateway::domain::discover::Reply reply;
               reply.process.pid = strong::process::id{ 42};
               reply.process.ipc = ipc_1;
               reply.correlation = correlation;
               coordinate.accumulate( reply, send);

               EXPECT_TRUE( send.sent == false);
               EXPECT_TRUE( coordinate.size() == 1);

               reply.process.pid = strong::process::id{ 77};
               reply.process.ipc = ipc_2;
               coordinate.accumulate( reply, send);

               EXPECT_TRUE( send.sent == true);
               EXPECT_TRUE( coordinate.size() == 0);
            }


         }


         TEST( common_message_coordinate, add_pid_42__remove_pid_42__expect__send)
         {
            common::unittest::Trace trace;

            local::Coordinate coordinate;
            local::Send send;

            auto correlation = uuid::make();

            {
               coordinate.add( correlation, strong::ipc::id{ uuid::make()}, send, std::vector< strong::process::id>{ strong::process::id{ 42}}) ;
               EXPECT_TRUE( send.sent == false);
            }

            {
               coordinate.remove( strong::process::id{ 42}, send);
               EXPECT_TRUE( send.sent == true) << CASUAL_NAMED_VALUE( coordinate);
            }


         }

      } // message

   } // common
} // casual
