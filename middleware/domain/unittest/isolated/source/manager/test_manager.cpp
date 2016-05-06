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
                     std::ifstream file;

                     while( ! file.is_open())
                     {
                        process::sleep( std::chrono::milliseconds{ 10});
                        file.open( environment::domain::singleton::file());
                     }

                     platform::ipc::id::type result;

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
domain:

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






      } // manager

   } // domain


} // casual
