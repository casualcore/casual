//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"


#include "domain/manager/unittest/process.h"
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
            constexpr auto configuration = R"(
domain:
   name: service-domain

   groups:
      - name: first
      - name: second
        dependencies: [ first]

   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ first]

   executables:
      - path: bin/casual-event-service-log
        arguments: [ --file, "${SERVICE_LOG_FILE}"]
        memberships: [ second]
        
)";

            struct Domain
            {
               Domain() = default;
               Domain( const char* configuration) 
                  : domain{ { configuration}} {}

               domain::manager::unittest::Process domain{ { local::configuration}};
            };

            auto ping_handle = []( auto& handle)
            {
               return common::communication::instance::ping( handle.ipc) == handle;
            };

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


         } // <unnamed>
      } // local

      TEST( event_service_log, reopen_log_file)
      {
         common::unittest::Trace trace;

         auto log_file = common::unittest::file::temporary::name( ".log");

         auto guard = common::environment::variable::scoped::set( "SERVICE_LOG_FILE", log_file.string());

         local::Domain domain;

         // we use the _singleton uuid_ that we know to address service-log.
         auto service_log_handle = common::communication::instance::fetch::handle( 0xc9d132c7249241c8b4085cc399b19714_uuid);

         // we know that the service-log is pingable after it has set up the event registration
         EXPECT_TRUE( local::ping_handle( service_log_handle));

         // produce metric to log-file
         EXPECT_TRUE( casual::domain::manager::api::state().executables.at( 0).alias == "casual-event-service-log");

         // we wait until we got something
         EXPECT_TRUE( local::file::wait::content( log_file)) << local::file::string( log_file);
         
         // move the file
         auto rotated = common::unittest::file::temporary::name( ".log");
         common::file::rename( log_file, rotated);

         // send hangup to service-log, to open the (new) file again
         common::signal::send( service_log_handle.pid, common::code::signal::hangup);

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

         local::Domain domain{ R"(
domain:
   name: service-domain

   groups:
      - name: first
      - name: second
        dependencies: [ first]
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ first]

   executables:
      - path: bin/casual-event-service-log
        arguments: [ --file, "${SERVICE_LOG_FILE}", --filter-exclusive, '^[.].*$']
        memberships: [ second]
        
)"};


         // we use the _singleton uuid_ that we know to address service-log.
         auto service_log_handle = common::communication::instance::fetch::handle( 0xc9d132c7249241c8b4085cc399b19714_uuid);

         // we know that the service-log is pingable after it has set up the event registration
         EXPECT_TRUE( local::ping_handle( service_log_handle));

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

         local::Domain domain{ R"(
domain:
   name: service-domain

   groups:
      - name: first
      - name: second
        dependencies: [ first]
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ first]

   executables:
      - path: bin/casual-event-service-log
        arguments: [ --file, "${SERVICE_LOG_FILE}", --filter-inclusive, '^[.].*$']
        memberships: [ second]
        
)"};


         // we use the _singleton uuid_ that we know to address service-log.
         auto service_log_handle = common::communication::instance::fetch::handle( 0xc9d132c7249241c8b4085cc399b19714_uuid);

         // we know that the service-log is pingable after it has set up the event registration
         EXPECT_TRUE( local::ping_handle( service_log_handle));

         // make sure we get som metrics 
         casual::domain::manager::api::state();

         // the call should match the filter, hence logged        
         EXPECT_TRUE( local::file::wait::content( log_file)) << local::file::string( log_file);
      }


   } // event
} // casual

