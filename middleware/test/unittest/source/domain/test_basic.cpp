//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"


#include "common/mockup/process.h"
#include "common/mockup/file.h"
#include "common/environment.h"
#include "common/communication/instance.h"

namespace casual
{
   using namespace common;

   namespace test
   {
      namespace domain
      {
         namespace local
         {
            namespace
            {
               namespace configuration
               {

                  std::vector< file::scoped::Path> files( const std::vector< std::string>& config)
                  {
                     return algorithm::transform( config, []( const std::string& c){
                        return file::scoped::Path{ mockup::file::temporary::content( ".yaml", c)};
                     });
                  }

                  std::string names( const std::vector< file::scoped::Path>& files)
                  {
                     return string::join( files, " ");
                  }

               } // configuration

               struct Manager
               {
                  Manager( const std::vector< std::string>& config)
                   : files( configuration::files( config)),
                     process{ "./home/bin/casual-domain-manager", {
                        "--event-queue", common::string::compose( common::communication::ipc::inbound::ipc()),
                        "--configuration-files", configuration::names( files),
                        "--no-auto-persist"
                     }}
                  {
                     //
                     // Wait for the domain to boot
                     //
                     unittest::domain::manager::wait( common::communication::ipc::inbound::device());
                  }

                  struct set_environment_variables_t
                  {
                     set_environment_variables_t()
                     {
                        // make sure we use our newly built stuff in the repo
                        environment::variable::set( "CASUAL_HOME", "./home");
                     }

                  } set_environment_variables;

                  struct remove_singleton_file_t
                  {
                     remove_singleton_file_t()
                     {
                        if( file::exists( environment::domain::singleton::file()))
                        {
                           file::remove( environment::domain::singleton::file());
                        }
                     }
                  } remove_singleton_file;

                  std::vector< file::scoped::Path> files;
                  mockup::Process process;
               };

            } // <unnamed>
         } // local


         TEST( test_domain_basic, empty_configuration)
         {
            // common::unittest::Trace trace;

            const auto configuration = R"(
domain:
  name: empty_configuration
)";

            local::Manager manager{ { configuration}};

            EXPECT_TRUE( communication::instance::ping( manager.process.handle().ipc) == manager.process.handle());
         }


      } // domain

   } // test
} // casual
