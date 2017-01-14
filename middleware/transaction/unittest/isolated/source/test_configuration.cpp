//!
//! casual
//!


#include <gtest/gtest.h>
#include "common/unittest.h"


#include "transaction/manager/manager.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"

#include "common/message/transaction.h"
#include "common/mockup/ipc.h"
#include "common/mockup/domain.h"
#include "common/internal/trace.h"
#include "common/internal/log.h"
#include "common/environment.h"


namespace casual
{

   namespace local
   {
      namespace
      {
         common::message::domain::configuration::Domain configuration()
         {
            common::message::domain::configuration::Domain domain;


            common::message::domain::configuration::transaction::Resource resource;
            resource.name = "rm1";
            resource.key = "rm-mockup";
            resource.instances = 3;
            resource.openinfo = "some open info 1";
            resource.closeinfo = "some close info 1";
            domain.transaction.resources.push_back( resource);

            resource.name = "rm2";
            resource.key = "rm-mockup";
            resource.instances = 3;
            resource.openinfo = "some open info 2";
            resource.closeinfo = "some close info 2";
            domain.transaction.resources.push_back( resource);

            return domain;
         }

         struct Domain
         {

            Domain() : manager{ configuration()}
            {
            }

            common::mockup::domain::Manager manager;
            common::mockup::domain::Broker broker;
         };

      } // <unnamed>
   } // local

   TEST( casual_transaction_configuration, configure_xa_config__expect_2_resources)
   {
      common::unittest::Trace trace;

      common::environment::variable::set( "CASUAL_RESOURCE_CONFIGURATION_FILE", "../example/resources/resources.yaml");

      local::Domain domain;

      transaction::State state( ":memory:");

      transaction::action::configure( state);

      ASSERT_TRUE( state.resource_properties.size() >= 2) << "state.xaConfig.size(): " << state.resource_properties.size();
      EXPECT_TRUE( state.resource_properties.at( "db2").xa_struct_name == "db2xa_switch_static_std");
      EXPECT_TRUE( state.resource_properties.at( "rm-mockup").xa_struct_name == "casual_mockup_xa_switch_static");
   }

   TEST( casual_transaction_configuration, configure_resource__expect_2_resources)
   {
      common::unittest::Trace trace;

      common::environment::variable::set( "CASUAL_RESOURCE_CONFIGURATION_FILE", "../example/resources/resources.yaml");

      local::Domain domain;

      transaction::State state( ":memory:");

      transaction::action::configure( state);

      ASSERT_TRUE( state.resources.size() == 2) << "state.resources: " << state.resources.size();
      EXPECT_TRUE( state.resources.at( 0).id > 0) << "id: " << state.resources.at( 0).id;
      EXPECT_TRUE( state.resources.at( 0).openinfo == "some open info 1");
      EXPECT_TRUE( state.resources.at( 0).name == "rm1");
      EXPECT_TRUE( state.resources.at( 1).id > 0) << "id: " << state.resources.at( 1).id;
      EXPECT_TRUE( state.resources.at( 1).closeinfo == "some close info 2");
      EXPECT_TRUE( state.resources.at( 1).name == "rm2");
   }



} // casual


