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
#include "common/environment/scoped.h"

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


            namespace file
            {
               auto empty = []( auto& path)
               {
                  std::ifstream file{ path};
                  return file.peek() == std::ifstream::traits_type::eof();
               };

               namespace wait
               {
                  auto content = []( auto& path)
                  {
                     auto count = 400;
                     while( file::empty( path) && count-- > 0)
                        common::process::sleep( std::chrono::milliseconds{ 4});

                     return count > 0;
                  };
               } // has

               auto string = []( auto& path)
               {
                  std::ifstream file{ path};
                  std::stringstream stream;
                  stream << file.rdbuf();
                  return std::move( stream).str();
               };
            } // file

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

         auto guard = common::environment::variable::scoped::set( "SERVICE_LOG_FILE", log_file.string());

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
         EXPECT_TRUE( local::file::wait::content( log_file)) << local::file::string( log_file);
         
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
            while( local::file::empty( log_file) && count-- > 0)
            {
               casual::domain::manager::api::state();
               common::process::sleep( std::chrono::milliseconds{ 2});
            }

            EXPECT_TRUE( count > 0);
         }
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
         EXPECT_TRUE( local::file::empty( log_file)) << local::file::string( log_file);
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
         EXPECT_TRUE( local::file::wait::content( log_file)) << local::file::string( log_file);
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
         EXPECT_TRUE( local::file::wait::content( log_file)) << local::file::string( log_file);
      }

   } // event
} // casual

