//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include <gtest/gtest.h>
#include "common/unittest.h"


#include "transaction/manager/manager.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"

#include "common/message/transaction.h"
#include "common/environment.h"

#include "domain/manager/unittest/process.h"

namespace casual
{
   namespace local
   {
      namespace
      {
         constexpr auto configuration = R"(
domain:
   name: transaction-configuration

   transaction:
      resources:
         - name: rm1
           key: rm-mockup
           instances: 3
           openinfo: some open info 1
           closeinfo: some close info 1
         - name: rm2
           key: rm-mockup
           instances: 3
           openinfo: some open info 2
           closeinfo: some close info 2
)";

         struct Domain
         {
            domain::manager::unittest::Process manager{ { configuration}};
            //common::mockup::domain::service::Manager service;
         };

      } // <unnamed>
   } // local

   TEST( casual_transaction_configuration, configure_xa_config__expect_2_resources)
   {
      common::unittest::Trace trace;

      common::environment::variable::set( "CASUAL_RESOURCE_CONFIGURATION_FILE", "../example/resources/resources.yaml");

      local::Domain domain;

      transaction::manager::State state( ":memory:");

      transaction::manager::action::configure( state);

      ASSERT_TRUE( state.resource_properties.size() >= 2) << "state.xaConfig.size(): " << state.resource_properties.size();
      EXPECT_TRUE( state.resource_properties.at( "db2").xa_struct_name == "db2xa_switch_static_std");
      EXPECT_TRUE( state.resource_properties.at( "rm-mockup").xa_struct_name == "casual_mockup_xa_switch_static");
   }

   TEST( casual_transaction_configuration, configure_resource__expect_2_resources)
   {
      common::unittest::Trace trace;

      common::environment::variable::set( "CASUAL_RESOURCE_CONFIGURATION_FILE", "../example/resources/resources.yaml");

      local::Domain domain;

      transaction::manager::State state( ":memory:");

      transaction::manager::action::configure( state);

      const common::strong::resource::id invalid;

      ASSERT_TRUE( state.resources.size() == 2) << "state.resources: " << state.resources.size();
      EXPECT_TRUE( state.resources.at( 0).id > invalid) << "id: " << state.resources.at( 0).id;
      EXPECT_TRUE( state.resources.at( 0).openinfo == "some open info 1");
      EXPECT_TRUE( state.resources.at( 0).name == "rm1");
      EXPECT_TRUE( state.resources.at( 1).id > invalid) << "id: " << state.resources.at( 1).id;
      EXPECT_TRUE( state.resources.at( 1).closeinfo == "some close info 2");
      EXPECT_TRUE( state.resources.at( 1).name == "rm2");
   }



} // casual


