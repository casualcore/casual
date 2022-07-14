//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/event/dispatch.h"

#include "domain/unittest/manager.h"
#include "casual/domain/manager/api/state.h"

#include "common/message/event.h"
#include "common/message/service.h"
#include "common/communication/instance.h"

#include "common/algorithm/is.h"


namespace casual
{
   using namespace common;

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
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
)";


            template< typename... Cs>
            auto domain( Cs&&... configurations)
            {
               return casual::domain::unittest::manager( local::configuration, std::forward< Cs>( configurations)...);
            }


            void shutdown()
            {
               communication::ipc::inbound::device().push( message::shutdown::Request{ process::handle()});
            }
         } // <unnamed>
      } // local

      TEST( event_dispatch, empty)
      {
         common::unittest::Trace trace;

         bool called_empty = false;

         event::dispatch( [ &called_empty]()
         {
            if( std::exchange( called_empty, true))
               return;
            
            // trigger end of _event dispatch_
            local::shutdown();
         });

         EXPECT_TRUE( called_empty);
      }

      TEST( event_dispatch, service_call)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         event::dispatch( [ executed = false]() mutable
         {
            // this is an idle 'callback', we need to ensure we're only calling one time...
            if( std::exchange( executed, true))
               return;

            // we call domain manager, that will trigger a call-event
            auto state = casual::domain::manager::api::state();
            EXPECT_TRUE( ! state.servers.empty());
            
         },
         []( model::service::Call&& call)
         {
            ASSERT_TRUE( call.metrics.size() == 1);
            auto& metric = call.metrics.at( 0);
            EXPECT_TRUE( metric.service.name == ".casual/domain/state");
            EXPECT_TRUE( metric.service.type == decltype(  metric.service.type)::sequential);
            // trigger end of _event dispatch_
            local::shutdown();
         });
      }

      TEST( event_dispatch, 5_service_call)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         constexpr auto calls = 5;
         auto count = 0;

         event::dispatch( 
         [ count = 0]() mutable
         {
            // we call domain manager, that will trigger 5 call-events
            // we need to make sure we only call exactly 5 times, since
            // we could get another invocation to 'idle'
            while( count++ < calls)
            {
               auto state = casual::domain::manager::api::state();
               EXPECT_TRUE( ! state.servers.empty());
            }
         },
         [ &count]( model::service::Call&& call)
         {
            count += call.metrics.size();
            EXPECT_TRUE( call.metrics.at( 0).service.name == ".casual/domain/state");
            // trigger end of _event dispatch_
            if( count == calls)
               local::shutdown();
         });

         EXPECT_TRUE( count == calls) << "count: " << count;
      }

      TEST( event_dispatch, 5_service_async_call__expect_pending)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         struct
         {
            std::vector< platform::time::unit> pending{};
         } state;

         constexpr auto count = 5;

         event::dispatch( 
         [executed = false]() mutable
         {
            // this is an idle 'callback', we need to ensure we're only calling one time...
            if( std::exchange( executed, true))
               return;
            
            auto lookup = []()
            {
               common::message::service::lookup::Request request{ common::process::handle()};
               request.requested = ".casual/domain/state";
               request.context.semantic = decltype( request.context.semantic)::no_busy_intermediate;

               return common::communication::device::async::call( 
                  common::communication::instance::outbound::service::manager::device(),
                  request);
            };

            std::vector< decltype( lookup())> lookups;

            common::algorithm::for_n< count>( [&]()
            { 
               lookups.push_back( lookup());
            });

            EXPECT_TRUE( lookups.size() == count);

            // fetch lookup and call service. 
            // this works since we consume from our inbound, hence we'll 'cache'
            // the replies from domain-manager, and we can consume them later 
            auto requests = common::algorithm::transform( lookups, []( auto& lookup)
            {
               auto reply = lookup.get( common::communication::ipc::inbound::device());
               EXPECT_TRUE( reply.state == decltype( reply.state)::idle);
               common::message::service::call::callee::Request request;
               request.process = common::process::handle();
               request.service = std::move( reply.service);
               request.pending = reply.pending;
               request.buffer.type = common::buffer::type::binary();
               
               return common::communication::device::async::call( 
                  reply.process.ipc,
                  request);
            });

            // consume the replies. 
            common::algorithm::for_each( requests, []( auto& request)
            {
               auto reply = request.get( common::communication::ipc::inbound::device());
               EXPECT_TRUE( reply.code.result == decltype( reply.code.result)::ok);
            });
         },
         [&state]( model::service::Call&& call)
         {
            common::algorithm::for_each( call.metrics, [&state]( auto& metric)
            {
               EXPECT_TRUE( metric.service.name == ".casual/domain/state");
               state.pending.push_back( metric.pending);
            });
            
            if( state.pending.size() == count)
               local::shutdown();
         });

         ASSERT_TRUE( state.pending.size() == count) << CASUAL_NAMED_VALUE( state.pending);

         common::algorithm::sort( state.pending);

         // we know it has to be pending for all except the first one..
         EXPECT_TRUE( state.pending.at( 0) == platform::time::unit::zero()) << CASUAL_NAMED_VALUE( state.pending);
         EXPECT_TRUE( state.pending.at( 1) > platform::time::unit::zero()) << CASUAL_NAMED_VALUE( state.pending);
      }


      TEST( event_dispatch, faked_concurrent_metric)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         event::dispatch( 
         [executed = false]() mutable
         {
            // this is an idle 'callback', we need to ensure we're only calling one time...
            if( std::exchange( executed, true))
               return;

            // we fake a concurrent metric message to service-manager
            // this only works since service-manager is (right now) very
            // laid back regarding if the services actually exist or not, 
            // if not, it just sends the metric event regardless... Might
            // change in the future.
            common::message::event::service::Calls message;
            message.metrics = {
               [](){ 
                  common::message::event::service::Metric metric;
                  metric.process = common::process::handle();
                  metric.type = decltype( metric.type)::concurrent;
                  metric.end = platform::time::clock::type::now();
                  metric.start = metric.end - std::chrono::milliseconds{ 2};
                  metric.service = "foo";

                  // no pending is set, we rely on initialization of event::service::Metric

                  return metric;
               }()
            };
            common::communication::device::blocking::send( 
               common::communication::instance::outbound::service::manager::device(), message);            
         },
         []( model::service::Call&& call)
         {
            ASSERT_TRUE( call.metrics.size() == 1) << CASUAL_NAMED_VALUE( call.metrics.size());
            auto& metric = call.metrics.at( 0);
            EXPECT_TRUE( metric.process.pid == common::process::id().value());
            EXPECT_TRUE( metric.pending == platform::time::unit::zero());
            EXPECT_TRUE( metric.service.name == "foo");
            EXPECT_TRUE( metric.service.type == decltype( metric.service.type)::concurrent);
            // we trigger shutdown, to trigger "end"
            local::shutdown();
         });
      }

   } // event
} // casual

