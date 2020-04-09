//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "gateway/manager/state.h"
#include "gateway/transform.h"

#include "configuration/model.h"
#include "configuration/model/load.h"

namespace casual
{
   using namespace common;
   namespace gateway
   {
      namespace manager
      {
         TEST( gateway_manager_state, empty)
         {
            unittest::Trace trace;

            State state;

            EXPECT_TRUE( state.running() == false) << CASUAL_NAMED_VALUE( state);
         }

         namespace local
         {
            namespace
            {
               template< typename T>
               auto state( T&& configuration)
               {
                  auto path = unittest::file::temporary::content( ".yaml", configuration);

                  return gateway::transform::state( configuration::model::load( { path}).gateway);
               }

               auto state()
               {
                  constexpr auto configuration = R"(
domain: 
   name: gateway-domain
   
   gateway:
      listeners: 
         - address: 127.0.0.1:6666
      connections:
         - address: 127.0.0.1:0001
         - address: 127.0.0.1:0002
         - address: 127.0.0.1:0003
         - address: 127.0.0.1:0004

)";
                  return state( configuration);
               }
               namespace running
               {
                  template< typename State>
                  auto state( State&& state)
                  {
                     int pid = 1;

                     for( auto& outbound : state.connections.outbound)
                     {
                        outbound.process.pid.underlaying() = pid++;
                        outbound.process.ipc.underlaying() = uuid::make();
                        outbound.runlevel = decltype( outbound.runlevel)::online;
                     }

                     return std::move( state);
                  }
               } // running
            } // <unnamed>
         } // local

         TEST( gateway_manager_state, connections_4__expect_order_1_to_4)
         {
            unittest::Trace trace;

            auto state = local::state();

            EXPECT_TRUE( state.connections.outbound.at( 0).order == 1);
            EXPECT_TRUE( state.connections.outbound.at( 1).order == 2);
            EXPECT_TRUE( state.connections.outbound.at( 2).order == 3);
            EXPECT_TRUE( state.connections.outbound.at( 3).order == 4);
         }

         TEST( gateway_manager_state, rediscover_no_running__expect_empty_task)
         {
            unittest::Trace trace;

            auto state = local::state();
            auto result = state.rediscover.tasks.add( state, "test-task");
            
            EXPECT_TRUE( result.description == "test-task") << CASUAL_NAMED_VALUE( result);
            EXPECT_TRUE( ! result.request.has_value()) << CASUAL_NAMED_VALUE( result);
         }


         TEST( gateway_manager_state, rediscover_4_running__expect_tasks)
         {
            unittest::Trace trace;

            auto state = local::running::state( local::state());
            auto result = state.rediscover.tasks.add( state, "test-task");

            auto correlation = result.correlation;

            ASSERT_TRUE( result.request.has_value()) << CASUAL_NAMED_VALUE( result);
            auto& request = result.request.value();

            {
               auto& outbound = state.connections.outbound.at( 3);
               EXPECT_TRUE( outbound.process.ipc == request.destination);
               EXPECT_TRUE( correlation == request.message.correlation);
            }

            auto make_reply = [&]( auto pid)
            {
               message::outbound::rediscover::Reply result;
               result.process.pid.underlaying() = pid;
               result.correlation = correlation;
               return result;
            };

            {
               auto result = state.rediscover.tasks.reply( state, make_reply( 4));
               auto& request = result.request.value();

               // should be the next one
               auto& outbound = state.connections.outbound.at( 2);
               EXPECT_TRUE( outbound.process.ipc == request.destination);
            }

            {
               auto result = state.rediscover.tasks.reply( state, make_reply( 3));
               auto& request = result.request.value();

               // should be the next one
               auto& outbound = state.connections.outbound.at( 1);
               EXPECT_TRUE( outbound.process.ipc == request.destination);
            }

            {
               auto result = state.rediscover.tasks.reply( state, make_reply( 2));
               auto& request = result.request.value();

               // should be the next one
               auto& outbound = state.connections.outbound.at( 0);
               EXPECT_TRUE( outbound.process.ipc == request.destination);
            } 

            {
               // last one, no request
               auto result = state.rediscover.tasks.reply( state, make_reply( 1));
               EXPECT_TRUE( ! result.request.has_value());

               EXPECT_TRUE( state.rediscover.tasks.empty());
            }          

         }


         TEST( gateway_manager_state, rediscover_1_running__remove_pid__expect_task_done)
         {
            unittest::Trace trace;

            constexpr auto configuration = R"(
domain: 
   name: gateway-domain
   
   gateway:
      listeners: 
         - address: 127.0.0.1:6666
      connections:
         - address: 127.0.0.1:0001
)";

            auto state = local::running::state( local::state( configuration));
            auto result = state.rediscover.tasks.add( state, "test-task");
            
            auto done = state.rediscover.tasks.remove( strong::process::id{ 1});

            ASSERT_TRUE( done.size() == 1);
            EXPECT_TRUE( done.at( 0).description == "test-task");
            EXPECT_TRUE( done.at( 0).correlation == result.correlation);
            EXPECT_TRUE( done.at( 0).outbounds.empty());

            EXPECT_TRUE( state.rediscover.tasks.empty());
         }
      } // manager
   } // gateway
   
} // casual