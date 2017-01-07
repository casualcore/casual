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

#include "configuration/message/domain.h"

namespace casual
{

   namespace local
   {
      namespace
      {
         configuration::message::Reply configuration()
         {
            configuration::message::Reply result;


            configuration::message::transaction::Resource resource;
            resource.name = "rm1";
            resource.key = "rm-mockup";
            resource.instances = 3;
            resource.openinfo = "some open info 1";
            resource.closeinfo = "some close info 1";
            result.domain.transaction.resources.push_back( resource);

            resource.name = "rm2";
            resource.key = "rm-mockup";
            resource.instances = 3;
            resource.openinfo = "some open info 2";
            resource.closeinfo = "some close info 2";
            result.domain.transaction.resources.push_back( resource);

            return result;
         }

         struct Domain
         {

            Domain() : manager{
               []( configuration::message::Request& r){

                  common::Trace trace{ "mockup transaction::manager::connect::Request", common::log::internal::debug};

                  auto reply = configuration();
                  reply.correlation = r.correlation;

                  common::mockup::ipc::eventually::send( r.process.queue, reply);
               }
            }
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

      local::Domain domain;

      transaction::State state( ":memory:");

      transaction::action::configure( state, "../example/resources/resources.yaml");

      ASSERT_TRUE( state.resource_properties.size() >= 2) << "state.xaConfig.size(): " << state.resource_properties.size();
      EXPECT_TRUE( state.resource_properties.at( "db2").xa_struct_name == "db2xa_switch_static_std");
      EXPECT_TRUE( state.resource_properties.at( "rm-mockup").xa_struct_name == "casual_mockup_xa_switch_static");
   }

   TEST( casual_transaction_configuration, configure_resource__expect_2_resources)
   {
      common::unittest::Trace trace;

      local::Domain domain;

      transaction::State state( ":memory:");

      transaction::action::configure( state, "../example/resources/resources.yaml");

      ASSERT_TRUE( state.resources.size() == 2);
      EXPECT_TRUE( state.resources.at( 0).id > 0) << "id: " << state.resources.at( 0).id;
      EXPECT_TRUE( state.resources.at( 0).openinfo == "some open info 1");
      EXPECT_TRUE( state.resources.at( 0).name == "rm1");
      EXPECT_TRUE( state.resources.at( 1).id > 0) << "id: " << state.resources.at( 1).id;
      EXPECT_TRUE( state.resources.at( 1).closeinfo == "some close info 2");
      EXPECT_TRUE( state.resources.at( 1).name == "rm2");
   }



} // casual


