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

         ASSERT_TRUE( local::fetch::service::log());

         // make sure we get som metrics 
         casual::domain::manager::api::state();

         auto parts = common::string::split( common::string::split( common::unittest::file::fetch::until::content( log_file), '\n').at( 0), '|');

         // .casual/domain/state||26450|d099514934e0411e998eee8e4e99472f||1719906802794796|1719906802795079|0|OK|S|ce80d8aa31982d16|9a0d3e22c30072a0

         EXPECT_TRUE( parts.at( 0) == ".casual/domain/state");
         EXPECT_TRUE( parts.at( 1).empty());
         EXPECT_TRUE( common::unittest::regex::match( parts.at( 2), "[0-9]+"));
         EXPECT_TRUE( common::unittest::regex::match( parts.at( 3), "[0-9a-f]{32}"));
         EXPECT_TRUE( parts.at( 4).empty()); // trid... need to check?
         EXPECT_TRUE( common::unittest::regex::match( parts.at( 5), "[0-9]{16}")); // start us
         EXPECT_TRUE( common::unittest::regex::match( parts.at( 6), "[0-9]{16}")); // end us
         EXPECT_TRUE( parts.at( 7) == "0"); // pending
         EXPECT_TRUE( parts.at( 8) == "OK"); // service result
         EXPECT_TRUE( parts.at( 9) == "S"); // sequential/parallel
         EXPECT_TRUE( common::unittest::regex::match( parts.at( 10), "[0-9a-f]{16}")); // execution span
         EXPECT_TRUE( common::unittest::regex::match( parts.at( 10), "[0-9a-f]{16}")); // parent execution span
      
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

