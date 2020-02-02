//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"


#include "domain/manager/unittest/process.h"
#include "casual/domain/manager/api/state.h"

#include "common/exception/casual.h"
#include "common/communication/instance.h"
#include "common/signal.h"
#include "common/environment.h"

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

   executables:
      - path: "${PWD}/bin/casual-event-service-log"
        arguments: [ --file, "${SERVICE_LOG_FILE}"]
        
)";

            struct Domain
            {
               Domain() = default;
               Domain( const char* configuration) 
                  : domain{ { configuration}} {}

               domain::manager::unittest::Process domain{ { local::configuration}};
            };


            namespace produce
            {
               // produce metric and make sure it has reach service-log
               // we use the fact that we know if we invoke the next part in the chain it's guaranteed that
               // the previous message has been processed.
               auto metric = []( auto& service_log)
               {
                  auto ping_handle = []( auto& handle)
                  {
                     return common::communication::instance::ping( handle.ipc) == handle;
                  };

                  // make sure we get som metrics 
                  casual::domain::manager::api::state();

                  // make sure domain manager has sent the metrics
                  EXPECT_TRUE( ping_handle( common::communication::instance::outbound::domain::manager::device().connector().process()));

                  // make sure service-manager has received and sent the metric to service-log
                  EXPECT_TRUE( ping_handle( common::communication::instance::outbound::service::manager::device().connector().process()));

                  // make sure service-log has received the metric from service-manager
                  EXPECT_TRUE( ping_handle( service_log));
               };
            } // produce

            namespace file
            {
               auto empty = []( auto& path)
               {
                  std::ifstream file{ path};
                  return file.peek() == std::ifstream::traits_type::eof();
               };

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

         common::environment::variable::set( "SERVICE_LOG_FILE", log_file);

         local::Domain domain;

         // we use the _singleton uuid_ that we know to address service-log.
         auto service_log_handle = common::communication::instance::fetch::handle( 0xc9d132c7249241c8b4085cc399b19714_uuid);

         // produce metric to log-file
         local::produce::metric( service_log_handle);
         
         // move the file
         auto rotated = common::unittest::file::temporary::name( ".log");
         common::file::move( log_file, rotated);

         // send hangup to service-log, to open the (new) file again
         common::signal::send( service_log_handle.pid, common::code::signal::hangup);

         // produce metric to the new log-file (with same name)
         local::produce::metric( service_log_handle);


         EXPECT_TRUE( ! local::file::empty( log_file)) << local::file::string( log_file);
         EXPECT_TRUE( ! local::file::empty( rotated)) << local::file::string( rotated);
      }


      TEST( event_service_log, discard_metric)
      {
         common::unittest::Trace trace;

         auto log_file = common::unittest::file::temporary::name( ".log");

         common::environment::variable::set( "SERVICE_LOG_FILE", log_file);

         local::Domain domain{ R"(
domain:
   name: service-domain

   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"

   executables:
      - path: "${PWD}/bin/casual-event-service-log"
        arguments: [ --file, "${SERVICE_LOG_FILE}", --discard, '^[.].*$']
        
)"};


         // we use the _singleton uuid_ that we know to address service-log.
         auto service_log_handle = common::communication::instance::fetch::handle( 0xc9d132c7249241c8b4085cc399b19714_uuid);

         local::produce::metric( service_log_handle);

         // the call should be discarded         
         EXPECT_TRUE( local::file::empty( log_file)) << local::file::string( log_file);
      }


      TEST( event_service_log, filter_metric)
      {
         common::unittest::Trace trace;

         auto log_file = common::unittest::file::temporary::name( ".log");

         common::environment::variable::set( "SERVICE_LOG_FILE", log_file);

         local::Domain domain{ R"(
domain:
   name: service-domain

   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"

   executables:
      - path: "${PWD}/bin/casual-event-service-log"
        arguments: [ --file, "${SERVICE_LOG_FILE}", --filter, '^[.].*$']
        
)"};


         // we use the _singleton uuid_ that we know to address service-log.
         auto service_log_handle = common::communication::instance::fetch::handle( 0xc9d132c7249241c8b4085cc399b19714_uuid);

         local::produce::metric( service_log_handle);

         // the call should match the filter, hence logged        
         EXPECT_TRUE( ! local::file::empty( log_file)) << local::file::string( log_file);
      }


   } // event
} // casual

