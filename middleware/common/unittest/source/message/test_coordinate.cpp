//!
//! casual
//!


#include "common/unittest.h"

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
                  void send( platform::ipc::id queue, M&& message)
                  {
                     sent = true;
                  }

                  Uuid correlation;
                  bool sent = false;
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
            coordinate.add( correlation, platform::ipc::id{ 666}, std::vector< common::process::Handle>{ { platform::process::id{ 10}, platform::ipc::id{ 10}}});

            EXPECT_TRUE( coordinate.policy().sent == false);
         }

         TEST( common_message_coordinate, send__accumulate__expect_coordination)
         {
            common::unittest::Trace trace;

            local::Coordinate coordinate;

            auto correlation = uuid::make();

            {
               coordinate.add( correlation, platform::ipc::id{ 666}, std::vector< common::process::Handle>{ { platform::process::id{ 42}, platform::ipc::id{ 42}}});
            }

            {
               gateway::domain::discover::Reply reply;
               reply.process.pid = platform::process::id{ 42};
               reply.process.queue = platform::ipc::id{ 42};
               reply.correlation = correlation;
               coordinate.accumulate( reply);
            }

            EXPECT_TRUE( coordinate.policy().sent == true) << "coordinate: " << coordinate; 
         }

         TEST( common_message_coordinate, send_2_destination__accumulate_1__expect_no_coordination)
         {
            common::unittest::Trace trace;

            local::Coordinate coordinate;

            auto correlation = uuid::make();

            {
               coordinate.add( correlation, platform::ipc::id{ 666}, std::vector< common::process::Handle>{ 
                  {  platform::process::id{ 42}, platform::ipc::id{ 42}}, {  platform::process::id{ 77}, platform::ipc::id{ 77}}});
            }

            {
               gateway::domain::discover::Reply reply;
               reply.process.pid = platform::process::id{ 42};
               reply.process.queue = platform::ipc::id{ 42};
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

            {
               coordinate.add( correlation, platform::ipc::id{ 666}, 
                  std::vector< common::process::Handle>{ 
                     {  platform::process::id{ 42}, platform::ipc::id{ 42}}, 
                     {  platform::process::id{ 77}, platform::ipc::id{ 77}}});
            }

            {
               gateway::domain::discover::Reply reply;
               reply.process.pid = platform::process::id{ 42};
               reply.process.queue = platform::ipc::id{ 42};
               reply.correlation = correlation;
               coordinate.accumulate( reply);

               EXPECT_TRUE( coordinate.policy().sent == false);
               EXPECT_TRUE( coordinate.size() == 1);

               reply.process.pid = platform::process::id{ 77};
               reply.process.queue = platform::ipc::id{ 77};
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
               coordinate.add( correlation, platform::ipc::id{ 666}, std::vector< platform::process::id>{ platform::process::id{ 42}}) ;
               EXPECT_TRUE( coordinate.policy().sent == false);
            }

            {
               coordinate.remove( platform::process::id{ 42});
               EXPECT_TRUE( coordinate.policy().sent == true) << "coordinate: " << coordinate;
            }


         }

      } // message

   } // common
} // casual
