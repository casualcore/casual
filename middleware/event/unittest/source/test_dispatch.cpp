//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/event/dispatch.h"

#include "domain/manager/unittest/process.h"
#include "domain/pending/message/send.h"
#include "casual/domain/manager/api/state.h"

#include "common/exception/casual.h"
#include "common/message/event.h"
#include "common/communication/instance.h"


namespace casual
{
   namespace event
   {

      namespace local
      {
         namespace
         {
            constexpr auto configuration = R"(
domain:
   name: service-domain

   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
)";

            struct Domain
            {
               Domain() = default;
               Domain( const char* configuration) 
                  : domain{ { configuration}} {}

               domain::manager::unittest::Process domain{ { local::configuration}};
            };

            namespace send
            {
               void shutdown()
               {
                  domain::pending::message::send( common::process::handle(), common::message::shutdown::Request{});
               }
            } // send
         } // <unnamed>
      } // local

      TEST( event_dispatch, empty)
      {
         common::unittest::Trace trace;

         try 
         {
            event::dispatch( []()
            {
               throw std::string{ "empty"};
            });
         }
         catch( const std::string& string)
         {
            EXPECT_TRUE( string == "empty");
         }
      }

      TEST( event_dispatch, service_call)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         event::dispatch( 
         []()
         {
            // we call domain manager, that will trigger a call-event
            auto state = casual::domain::manager::api::state();
            EXPECT_TRUE( ! state.servers.empty());
            
         },
         []( model::service::Call&& call)
         {
            ASSERT_TRUE( call.metrics.size() == 1);
            EXPECT_TRUE( call.metrics.at( 0).service.name == ".casual/domain/state");
            // we send shotdown to our self, to trigger "end"
            local::send::shutdown();
         });
      }

      TEST( event_dispatch, 5_service_call)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto count = 0;

         event::dispatch( 
         [calls = 5]() mutable
         {
            // we call domain manager, that will trigger 5 call-events
            // we need to make sure we only call exactly 5 times, since
            // we could get another invocation to 'idle'
            while( calls-- > 0)
            {
               auto state = casual::domain::manager::api::state();
               EXPECT_TRUE( ! state.servers.empty());
            }
         },
         [&count]( model::service::Call&& call)
         {
            count += call.metrics.size();
            EXPECT_TRUE( call.metrics.at( 0).service.name == ".casual/domain/state");
            // we send shotdown to our self, to trigger "end"
            if( count == 5)
               local::send::shutdown();
         });

         EXPECT_TRUE( count == 5) << "count: " << count;
      }


      TEST( event_dispatch, faked_concurrent_metric)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         event::dispatch( 
         []()
         {
            // we fake a concurent metric message to service-manager
            // this only works since service-manager is (right now) very
            // laid back regarding if the services actually exist or not, 
            // if not, it just sends the metric event regardless... Might
            // change in the future.
            common::message::event::service::Calls message;
            message.metrics = {
               [](){ 
                  common::message::event::service::Metric metric;
                  metric.process = common::process::handle();
                  metric.end = platform::time::clock::type::now();
                  metric.start = metric.end - std::chrono::milliseconds{ 2};
                  metric.service = "foo";

                  // no pending is set, we rely on initialization of event::service::Metric

                  return metric;
               }()
            };
            common::communication::ipc::blocking::send( 
               common::communication::instance::outbound::service::manager::device(), message);            
         },
         []( model::service::Call&& call)
         {
            ASSERT_TRUE( call.metrics.size() == 1) << CASUAL_NAMED_VALUE( call.metrics.size());
            EXPECT_TRUE( call.metrics.at( 0).process.pid == common::process::id().value());
            EXPECT_TRUE( call.metrics.at( 0).pending == platform::time::unit::zero());
            EXPECT_TRUE( call.metrics.at( 0).service.name == "foo");
            // we send shotdown to our self, to trigger "end"
            local::send::shutdown();
         });
      }

   } // event
} // casual

