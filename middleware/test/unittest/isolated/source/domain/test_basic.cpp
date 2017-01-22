//!
//! casual 
//!

#include "common/unittest.h"


#include "common/mockup/process.h"
#include "common/mockup/file.h"
#include "common/environment.h"

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
                     return range::transform( config, []( const std::string& c){
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
                     process{ "./bin/casual-domain-manager", {
                        "--configuration-files", configuration::names( files),
                        "--no-auto-persist"
                     }}
                  {

                  }

                  struct set_environment_variables_t
                  {
                     set_environment_variables_t()
                     {
                        environment::variable::set( "CASUAL_HOME", "./home");
                        environment::variable::set( environment::variable::name::domain::home(), ".casual/test-domain");
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
            const std::string configuration = R"(
domain:
  name: empty_configuration
)";

            local::Manager manager{ { configuration}};

            EXPECT_TRUE( process::ping( manager.process.handle().queue) == manager.process.handle());
         }


      } // domain

   } // test
} // casual
