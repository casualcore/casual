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

   } // event
} // casual

