//!
//! unittest_envrionment.cpp
//!
//! Created on: May 29, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>


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

                  log::stream::get( log::category::Type::casual_debug) << "mockup::unittest::Environment::SetUp";


                  directory::create( domain_path);

                  log::stream::get( log::category::Type::casual_debug) << environment::variable::name::domain::home() << " set to: " << environment::variable::get( environment::variable::name::domain::home()) << std::endl;
                  log::stream::get( log::category::Type::casual_debug)  << "environment::directory::domain(): " <<  environment::directory::domain() << std::endl;


                  if( ! directory::create( environment::domain::singleton::path()))
                  {
                     log::stream::get( log::category::Type::error) << "failed to create domain singleton directory\n";
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


const auto registration = ::testing::AddGlobalTestEnvironment( new casual::common::mockup::unittest::Environment());



