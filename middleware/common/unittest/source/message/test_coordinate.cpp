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
                  void accumulate( M&& request, R&& reply)
                  {
                     correlation = request.correlation;
                  }

                  template< typename M>
                  void send( strong::ipc::id queue, M&& message)
                  {
                     sent = true;
                  }

                  Uuid correlation;
                  bool sent = false;

                  CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
                  {
                     CASUAL_SERIALIZE( correlation);
                     CASUAL_SERIALIZE( sent);
                  })
               };

               using Coordinate = message::Coordinate< Policy>;

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
            coordinate.add( correlation, strong::ipc::id{ uuid::make()}, std::vector< common::process::Handle>{ { strong::process::id{ 10}, strong::ipc::id{ uuid::make()}}});

            EXPECT_TRUE( coordinate.policy().sent == false);
         }

         TEST( common_message_coordinate, send__accumulate__expect_coordination)
         {
            common::unittest::Trace trace;

            local::Coordinate coordinate;

            const auto correlation = uuid::make();
            const auto ipc = strong::ipc::id{ uuid::make()};

            {
               coordinate.add( correlation, strong::ipc::id{ uuid::make()}, std::vector< common::process::Handle>{ { strong::process::id{ 42}, ipc}});
            }

            {
               gateway::domain::discover::Reply reply;
               reply.process.pid = strong::process::id{ 42};
               reply.process.ipc = ipc;
               reply.correlation = correlation;
               coordinate.accumulate( reply);
            }

            EXPECT_TRUE( coordinate.policy().sent == true) << CASUAL_NAMED_VALUE( coordinate); 
         }

         TEST( common_message_coordinate, send_2_destination__accumulate_1__expect_no_coordination)
         {
            common::unittest::Trace trace;

            local::Coordinate coordinate;

            auto correlation = uuid::make();
            const auto ipc_1 = strong::ipc::id{ uuid::make()};
            const auto ipc_2 = strong::ipc::id{ uuid::make()};

            {
               coordinate.add( correlation, strong::ipc::id{ uuid::make()}, std::vector< common::process::Handle>{ 
                  {  strong::process::id{ 42}, ipc_1}, {  strong::process::id{ 77}, ipc_2}});
            }

            {
               gateway::domain::discover::Reply reply;
               reply.process.pid = strong::process::id{ 42};
               reply.process.ipc = ipc_1;
               reply.correlation = correlation;
               coordinate.accumulate( reply);
            }

            EXPECT_TRUE( coordinate.policy().sent == false);
            EXPECT_TRUE( coordinate.size() == 1);
         }

         TEST( common_message_coordinate, send_2_destination__accumulate_2__expect_coordination)
         {
            common::unittest::Trace trace;

            local::Coordinate coordinate;

            auto correlation = uuid::make();

            const auto ipc_1 = strong::ipc::id{ uuid::make()};
            const auto ipc_2 = strong::ipc::id{ uuid::make()};

            {
               coordinate.add( correlation, strong::ipc::id{ uuid::make()}, 
                  std::vector< common::process::Handle>{ 
                     {  strong::process::id{ 42}, ipc_1}, 
                     {  strong::process::id{ 77}, ipc_2}});
            }

            {
               gateway::domain::discover::Reply reply;
               reply.process.pid = strong::process::id{ 42};
               reply.process.ipc = ipc_1;
               reply.correlation = correlation;
               coordinate.accumulate( reply);

               EXPECT_TRUE( coordinate.policy().sent == false);
               EXPECT_TRUE( coordinate.size() == 1);

               reply.process.pid = strong::process::id{ 77};
               reply.process.ipc = ipc_2;
               coordinate.accumulate( reply);

               EXPECT_TRUE( coordinate.policy().sent == true);
               EXPECT_TRUE( coordinate.size() == 0);
            }


         }


         TEST( common_message_coordinate, add_pid_42__remove_pid_42__expect__send)
         {
            common::unittest::Trace trace;

            local::Coordinate coordinate;

            auto correlation = uuid::make();

            {
               coordinate.add( correlation, strong::ipc::id{ uuid::make()}, std::vector< strong::process::id>{ strong::process::id{ 42}}) ;
               EXPECT_TRUE( coordinate.policy().sent == false);
            }

            {
               coordinate.remove( strong::process::id{ 42});
               EXPECT_TRUE( coordinate.policy().sent == true) << CASUAL_NAMED_VALUE( coordinate);
            }


         }

      } // message

   } // common
} // casual
