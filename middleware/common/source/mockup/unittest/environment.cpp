//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"


#include "common/environment.h"
#include "common/uuid.h"
#include "common/file.h"
#include "common/log.h"
#include "common/domain.h"

#include "common/mockup/ipc.h"

// std
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
            constexpr auto repository_root = "CASUAL_REPOSITORY_ROOT";

            struct Environment : public ::testing::Environment
            {
               void SetUp() override
               {
                  log::line( log::stream::get( "casual.mockup"), "mockup::unittest::Environment::SetUp");


                  if( ! environment::variable::exists( environment::variable::name::home())
                     && environment::variable::exists( repository_root))
                  {
                     environment::variable::set( environment::variable::name::home(), 
                        environment::variable::get( repository_root) + "/middleware/test/home/bin");

                     common::log::line( 
                        log::stream::get( "casual.mockup"), 
                        environment::variable::name::home(), ": ", 
                        environment::variable::get( environment::variable::name::home()));
                  }
                  
                  std::string domain_path = environment::directory::temporary() + "/casual/unittest";

                  environment::variable::set( environment::variable::name::domain::home(), domain_path);

                  domain::identity( domain::Identity{ "unittest-domain"});

                  

                  directory::create( domain_path);

                  log::line( log::stream::get( "casual.mockup"), environment::variable::name::domain::home(), " set to: ", environment::variable::get( environment::variable::name::domain::home()));
                  log::line( log::stream::get( "casual.mockup"), "environment::directory::domain(): ", environment::directory::domain());


                  if( ! directory::create( environment::domain::singleton::path()))
                  {
                     log::line( log::stream::get( "error"), "failed to create domain singleton directory");
                  }
               }

               void TearDown() override
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



