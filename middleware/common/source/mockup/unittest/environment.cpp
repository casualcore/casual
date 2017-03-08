//!
//! casual
//!

#include "common/unittest.h"


#include "common/environment.h"
#include "common/uuid.h"
#include "common/file.h"
#include "common/internal/log.h"
#include "common/trace.h"
#include "common/domain.h"

#include "common/mockup/ipc.h"


#include <iostream>
#include <fstream>

namespace casual
{

   namespace common
   {
      namespace mockup
      {
         namespace unittest
         {
            struct Environment : public ::testing::Environment
            {
               void SetUp() override
               {
                  std::string domain_path;

                  if( environment::variable::exists( "CASUAL_BUILD_HOME"))
                  {
                     domain_path =  environment::variable::get( "CASUAL_BUILD_HOME") + "/.casual/unittest" ;
                  }
                  else
                  {
                     domain_path =  environment::directory::temporary() + "/.casual/unittest" ;
                  }
                  environment::variable::set( environment::variable::name::domain::home(), domain_path);

                  domain::identity( domain::Identity{ "unittest-domain"});

                  log::stream::get( "casual.mockup") << "mockup::unittest::Environment::SetUp" << std::endl;


                  directory::create( domain_path);

                  log::stream::get( "casual.mockup") << environment::variable::name::domain::home() << " set to: " << environment::variable::get( environment::variable::name::domain::home()) << std::endl;
                  log::stream::get( "casual.mockup")  << "environment::directory::domain(): " <<  environment::directory::domain() << std::endl;


                  if( ! directory::create( environment::domain::singleton::path()))
                  {
                     log::stream::get( "error") << "failed to create domain singleton directory\n";
                  }
               }

               virtual void TearDown() override
               {

               }

            private:

            };




         } // unittest
      } // mockup
   } // common

} // casual

namespace 
{
   const auto registration CASUAL_OPTION_UNUSED = ::testing::AddGlobalTestEnvironment( new casual::common::mockup::unittest::Environment());
}




