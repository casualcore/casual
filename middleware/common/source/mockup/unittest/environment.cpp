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
                  // make sure we've got the paths and stuff set up before we log any thing
                  {
                     std::string domain_path = environment::directory::temporary() + "/casual/unittest";

                     environment::variable::set( environment::variable::name::domain::home(), domain_path);

                     domain::identity( domain::Identity{ "unittest-domain"});

                     directory::create( domain_path);
                  }

                  auto& stream = log::stream::get( "casual.mockup");

                  log::line( stream, "mockup::unittest::Environment::SetUp");

                  log::line( stream, environment::variable::name::domain::home(), " set to: ", environment::variable::get( environment::variable::name::domain::home()));
                  log::line( stream, "environment::directory::domain(): ", environment::directory::domain());

                  if( ! environment::variable::exists( environment::variable::name::home())
                     && environment::variable::exists( repository_root))
                  {
                     environment::variable::set( environment::variable::name::home(), 
                        environment::variable::get( repository_root) + "/middleware/test/home/bin");

                     common::log::line( 
                        stream, 
                        environment::variable::name::home(), ": ", 
                        environment::variable::get( environment::variable::name::home()));
                  }

                  if( ! directory::create( environment::domain::singleton::path()))
                  {
                     log::line( log::stream::get( "error"), "failed to create domain singleton directory");
                  }
               }

               void TearDown() override
               {

               }
            };

         } // unittest
      } // mockup
   } // common

} // casual

namespace 
{
   const auto registration CASUAL_OPTION_UNUSED = ::testing::AddGlobalTestEnvironment( new casual::common::mockup::unittest::Environment());
}



