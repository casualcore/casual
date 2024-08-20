//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"


#include "domain/unittest/manager.h"
#include "casual/domain/manager/api/state.h"

#include "common/communication/instance.h"
#include "common/communication/select.h"
#include "common/signal.h"
#include "common/result.h"
#include "common/environment.h"
#include "common/message/event.h"

namespace casual
{
   namespace event
   {

      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto base = R"(
domain:
   name: service-log
   groups:
      - name: first
      - name: second
        dependencies: [ first]
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ first]        
)";
            } // configuration


            namespace fetch::service
            {
               auto log()
               {
                  // we use the _singleton uuid_ that we know to address service-log.
                  return common::communication::instance::fetch::handle( 0xc9d132c7249241c8b4085cc399b19714_uuid);
               } 
            } // fetch::service


         } // <unnamed>
      } // local

      TEST( event_service_log, reopen_log_file)
      {
         common::unittest::Trace trace;

         auto log_file = common::unittest::file::temporary::name( ".log");

         common::environment::variable::set( "SERVICE_LOG_FILE", log_file.string());

         auto domain = domain::unittest::manager( local::configuration::base, R"(
domain:
   executables:
      - path: bin/casual-event-service-log
        arguments: [ --file, "${SERVICE_LOG_FILE}"]
        memberships: [ second]
)");

         auto handle = local::fetch::service::log();

         // produce metric to log-file
         EXPECT_TRUE( casual::domain::manager::api::state().executables.at( 0).alias == "casual-event-service-log");

         // we wait until we got something
         EXPECT_TRUE( ! common::unittest::file::fetch::until::content( log_file).empty()) << common::unittest::file::fetch::content( log_file);
         
         // move the file
         auto rotated = common::unittest::file::temporary::name( ".log");
         common::file::rename( log_file, rotated);

         // send hangup to service-log, to open the (new) file again
         common::signal::send( handle.pid, common::code::signal::hangup);

         {
            // produce metric to log-file
            casual::domain::manager::api::state();

            // it's not deterministic when the signal arrives, so we need to "poll".
            auto count = 1000;
            while( common::unittest::file::empty( log_file) && count-- > 0)
            {
               casual::domain::manager::api::state();
               common::process::sleep( std::chrono::milliseconds{ 2});
            }

            EXPECT_TRUE( count > 0);
         }
      }

      TEST( event_service_log, format)
      {
         common::unittest::Trace trace;

         auto log_file = common::unittest::file::temporary::name( ".log");

         common::environment::variable::set( "SERVICE_LOG_FILE", log_file.string());

         auto domain = domain::unittest::manager( local::configuration::base, R"(
domain:
   executables:
      - path: bin/casual-event-service-log
        arguments: [ --file, "${SERVICE_LOG_FILE}"]
        memberships: [ second]   
)");

         auto service_log =  local::fetch::service::log();

         ASSERT_TRUE( service_log);

         const auto metric = common::message::event::service::Metric{
            .span = common::strong::execution::span::id::generate(),
            .service = "some/service",
            .parent = { .span = common::strong::execution::span::id::generate(), .service = "some/parent/service"} ,
            .type = common::message::event::service::metric::Type::sequential,
            .process = common::process::handle(),
            .correlation = common::strong::correlation::id::generate(),
            .execution = common::strong::execution::id::generate(),
            .trid = common::transaction::id::create( common::process::handle()),
            .start = platform::time::clock::type::now() - std::chrono::milliseconds{ 42},
            .end = platform::time::clock::type::now(),
            .pending = std::chrono::milliseconds{ 6},
            .code = common::service::Code{ .result = common::code::xatmi::ok, .user = 42},
         };

         // send metrics
         {
            common::message::event::service::Calls event;
            event.metrics.push_back( metric);
            ASSERT_TRUE( common::communication::device::blocking::send( service_log.ipc, event));
         }


         auto content = common::unittest::file::fetch::until::content( log_file);

         //EXPECT_TRUE( false) << CASUAL_NAMED_VALUE( content);

         auto parts = common::string::split( common::string::split( content, '\n').at( 0), '|');

         // some/service|some/parent/service|90053|0bd2c4b424e34e2296c25938766eedbf|10459c8a34e6422a9d5f584856db6701:a6f10817cdd94f3987985a8195e8f927:42:90053|1724141674147439|1724141674189439|6000|OK|S|cdb2b8d804d6e205|9bfeec3308a2a9cd|42

         EXPECT_TRUE( parts.at( 0) == "some/service");
         EXPECT_TRUE( parts.at( 1) == "some/parent/service");
         EXPECT_TRUE( parts.at( 2) == common::string::to( metric.process.pid));
         EXPECT_TRUE( parts.at( 3) == common::string::to( metric.execution));
         EXPECT_TRUE( parts.at( 4) == common::string::to( metric.trid));
         EXPECT_TRUE( parts.at( 5) == common::string::to( std::chrono::duration_cast< std::chrono::microseconds>( metric.start.time_since_epoch()).count()));
         EXPECT_TRUE( parts.at( 6) == common::string::to( std::chrono::duration_cast< std::chrono::microseconds>( metric.end.time_since_epoch()).count()));
         EXPECT_TRUE( parts.at( 7) == "6000"); // pending
         EXPECT_TRUE( parts.at( 8) == "OK"); // service result
         EXPECT_TRUE( parts.at( 9) == "S"); // sequential/parallel
         EXPECT_TRUE( parts.at( 10) == common::string::to( metric.span));
         EXPECT_TRUE( parts.at( 11) == common::string::to( metric.parent.span));
         EXPECT_TRUE( parts.at( 12) == "42"); // user return code
      }


      TEST( event_service_log, filter_exclusive)
      {
         common::unittest::Trace trace;

         auto log_file = common::unittest::file::temporary::name( ".log");

         common::environment::variable::set( "SERVICE_LOG_FILE", log_file.string());

         auto domain = domain::unittest::manager( local::configuration::base, R"(
domain:
   executables:
      - path: bin/casual-event-service-log
        arguments: [ --file, "${SERVICE_LOG_FILE}", --filter-exclusive, '^[.].*$']
        memberships: [ second]
)");

         ASSERT_TRUE( local::fetch::service::log());

         // produce metric to log-file
         casual::domain::manager::api::state();

         // give it some time for the metric to arrive.
         common::process::sleep( std::chrono::milliseconds{ 1});

         // the metric should be discarded.
         EXPECT_TRUE( common::unittest::file::empty( log_file)) << common::unittest::file::fetch::content( log_file);
      }


      TEST( event_service_log, filter_inclusive)
      {
         common::unittest::Trace trace;

         auto log_file = common::unittest::file::temporary::name( ".log");

         common::environment::variable::set( "SERVICE_LOG_FILE", log_file.string());

         auto domain = domain::unittest::manager( local::configuration::base, R"(
domain:
   executables:
      - path: bin/casual-event-service-log
        arguments: [ --file, "${SERVICE_LOG_FILE}", --filter-inclusive, '^[.].*$']
        memberships: [ second]   
)");

         ASSERT_TRUE( local::fetch::service::log());

         // make sure we get som metrics 
         casual::domain::manager::api::state();

         // the call should match the filter, hence logged        
         EXPECT_TRUE( ! common::unittest::file::fetch::until::content( log_file).empty()) << common::unittest::file::fetch::content( log_file);
      }

      TEST( event_service_log, filter_inclusive_exclusive)
      {
         common::unittest::Trace trace;

         auto log_file = common::unittest::file::temporary::name( ".log");

         common::environment::variable::set( "SERVICE_LOG_FILE", log_file.string());

         auto domain = domain::unittest::manager( local::configuration::base, R"(
domain:
   executables:
      - path: bin/casual-event-service-log
        arguments: [ --file, "${SERVICE_LOG_FILE}", --filter-inclusive, '.*state', --filter-exclusive, ".*foo.*"]
        memberships: [ second]
)");

         ASSERT_TRUE( local::fetch::service::log());

         // make sure we get som metrics 
         casual::domain::manager::api::state();

         // the call should match the filter, hence logged        
         EXPECT_TRUE( ! common::unittest::file::fetch::until::content(log_file).empty()) << common::unittest::file::fetch::content( log_file);
      }

   } // event
} // casual

