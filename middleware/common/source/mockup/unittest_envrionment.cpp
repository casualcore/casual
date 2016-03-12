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
#include "common/domain.h"

#include "common/mockup/ipc.h"


#include <iostream>

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
                  domain::identity( domain::Identity{ "unittest-domain"});

                  std::string domain_path;

                  if( environment::variable::exists( "CASUAL_BUILD_HOME"))
                  {
                     domain_path =  environment::variable::get( "CASUAL_BUILD_HOME") + "/.unittest_domain_home" ;
                  }
                  else
                  {
                     domain_path =  environment::directory::temporary() + "/casual_unittest_domain_home" ;
                  }
                  environment::variable::set( environment::variable::name::domain::home(), domain_path);

                  directory::create( domain_path);

                  log::debug << environment::variable::name::domain::home() << " set to: " << environment::variable::get( environment::variable::name::domain::home()) << std::endl;
                  log::debug  << "environment::directory::domain(): " <<  environment::directory::domain() << std::endl;


                  // poke all global queues
                  mockup::ipc::clear();

                  log::debug << "mockup broker queue id: " << mockup::ipc::broker::queue().id() << std::endl;
                  log::debug << "broker queue id: " << communication::ipc::broker::id() << std::endl;
                  log::debug << "mockup TM queue id: " << mockup::ipc::transaction::manager::queue().id() << std::endl;

               }

               virtual void TearDown() override
               {

               }

            };




         } // unittest
      } // mockup
   } // common

} // casual


const auto registration = ::testing::AddGlobalTestEnvironment( new casual::common::mockup::unittest::Environment());



