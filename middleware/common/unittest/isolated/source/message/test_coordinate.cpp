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
                  template< typename M, typename R>
                  void operator() ( M&& request, R&& reply)
                  {
                     correlation = request.correlation;
                  }

                  template< typename M>
                  void operator () ( platform::ipc::id::type queue, M&& message)
                  {
                     sent = true;
                  }

                  Uuid correlation;
                  bool sent = false;
               };

               using Coordinate = message::Coordinate< gateway::domain::discover::Reply, Policy>;

            } // <unnamed>
         } // local



         TEST( common_message_coordinate, instansation)
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
            coordinate.add( correlation, 666, std::vector< common::process::Handle>{ {  10, 10}});

            EXPECT_TRUE( coordinate.policy().sent == false);
         }

         TEST( common_message_coordinate, send__accumulate__expect_coordination)
         {
            common::unittest::Trace trace;

            local::Coordinate coordinate;

            auto correlation = uuid::make();

            {
               coordinate.add( correlation, 666, std::vector< common::process::Handle>{ {  42, 42}});
            }

            {
               gateway::domain::discover::Reply reply;
               reply.process.pid = 42;
               reply.process.queue = 42;
               reply.correlation = correlation;
               coordinate.accumulate( reply);
            }

            EXPECT_TRUE( coordinate.policy().sent == true);
         }

         TEST( common_message_coordinate, send_2_destination__accumulate_1__expect_no_coordination)
         {
            common::unittest::Trace trace;

            local::Coordinate coordinate;

            auto correlation = uuid::make();

            {
               coordinate.add( correlation, 666, std::vector< common::process::Handle>{ {  42, 42}, {  77, 77}});
            }

            {
               gateway::domain::discover::Reply reply;
               reply.process.pid = 42;
               reply.process.queue = 42;
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
               coordinate.add( correlation, 666, std::vector< common::process::Handle>{ {  42, 42}, {  77, 77}});
            }

            {
               gateway::domain::discover::Reply reply;
               reply.process.pid = 42;
               reply.process.queue = 42;
               reply.correlation = correlation;
               coordinate.accumulate( reply);

               EXPECT_TRUE( coordinate.policy().sent == false);
               EXPECT_TRUE( coordinate.size() == 1);

               reply.process.pid = 77;
               reply.process.queue = 77;
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
               coordinate.add( correlation, 666, std::vector< platform::pid::type>{ 42});
               EXPECT_TRUE( coordinate.policy().sent == false);
            }

            {
               coordinate.remove( 42);
               EXPECT_TRUE( coordinate.policy().sent == true);
            }


         }

      } // message

   } // common
} // casual
