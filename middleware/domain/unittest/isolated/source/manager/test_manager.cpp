//!
//! casual 
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "domain/manager/state.h"
#include "domain/manager/configuration.h"
#include "domain/manager/manager.h"


#include "common/string.h"
#include "common/environment.h"
#include "common/mockup/file.h"
#include "common/mockup/process.h"


#include <fstream>

namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {
         namespace local
         {
            namespace
            {
               namespace configuration
               {

                  std::vector< file::scoped::Path> files( const std::vector< std::string>& config)
                  {
                     return range::transform( config, []( const std::string& c){
                        return file::scoped::Path{ mockup::file::temporary( ".yaml", c)};
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
                     process{ "./bin/casual-domain-manager", {
                        "--configuration-files", configuration::names( files),
                        "--bare",
                     }}
                  {

                  }


                  std::vector< file::scoped::Path> files;
                  mockup::Process process;
               };

               namespace manager
               {

                  platform::ipc::id::type ipc()
                  {
                     std::ifstream file{ environment::domain::singleton::file()};

                     while( ! file.is_open())
                     {
                        process::sleep( std::chrono::milliseconds{ 10});
                        file.open( environment::domain::singleton::file());
                     }

                     platform::ipc::id::type result = 0;

                     file >> result;

                     log::debug << "manager ipc: " << result << '\n';

                     return result;
                  }

               } // manager


               namespace configuration
               {

                  std::string empty()
                  {
                     return R"(
domain: {}

)";

                  }

                  std::string echo()
                  {
                     return R"(
domain:
  executables:
    - path: echo
      instances: 4    

)";
                  }

                  std::string echo_restart()
                  {
                     return R"(
domain:
  executables:
    - path: echo
      instances: 4
      arguments: [poop]
      restart: true    

)";
                  }


                  std::string sleep()
                  {
                     return R"(
domain:
  executables:
    - path: sleep
      arguments: [100]
      instances: 4    

)";
                  }



               } // configuration

            } // <unnamed>
         } // local

         TEST( casual_domain_manager, empty_configuration__expect_boot)
         {
            CASUAL_UNITTEST_TRACE();

            EXPECT_NO_THROW( {
               local::Manager manager{ { local::configuration::empty()}};
               process::ping( local::manager::ipc());

            });
         }

         TEST( casual_domain_manager, echo_configuration__expect_boot)
         {
            CASUAL_UNITTEST_TRACE();

            EXPECT_NO_THROW( {
               local::Manager manager{ { local::configuration::echo()}};
               process::ping( local::manager::ipc());

            });
         }

         /*
         TEST( casual_domain_manager, echo_restart_configuration__expect_boot)
         {
            CASUAL_UNITTEST_TRACE();

            EXPECT_NO_THROW( {
               local::Manager manager{ { local::configuration::echo_restart()}};
               process::ping( local::manager::ipc());

            });
         }
         */

         TEST( casual_domain_manager, sleep_configuration__expect_boot)
         {
            CASUAL_UNITTEST_TRACE();

            EXPECT_NO_THROW( {
               local::Manager manager{ { local::configuration::sleep()}};
               process::ping( local::manager::ipc());

            });
         }





      } // manager

   } // domain


} // casual
