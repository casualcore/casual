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

namespace casual
{

   namespace local
   {
      namespace
      {
         common::message::domain::configuration::transaction::resource::Reply configuration()
         {
            common::message::domain::configuration::transaction::resource::Reply result;


            common::message::domain::configuration::transaction::Resource resource;
            resource.id = 1;
            resource.key = "rm-mockup";
            resource.instances = 3;
            resource.openinfo = "some open info 1";
            resource.closeinfo = "some close info 1";
            result.resources.push_back( resource);

            resource.id = 2;
            resource.key = "rm-mockup";
            resource.instances = 3;
            resource.openinfo = "some open info 2";
            resource.closeinfo = "some close info 2";
            result.resources.push_back( resource);

            return result;
         }

         struct Domain
         {

            Domain() : manager{
               []( common::message::domain::configuration::transaction::resource::Request& r){

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
      CASUAL_UNITTEST_TRACE();

      local::Domain domain;

      transaction::State state( ":memory:");

      transaction::action::configure( state, "../example/resources/resources.yaml");

      ASSERT_TRUE( state.xa_switch_configuration.size() >= 2) << "state.xaConfig.size(): " << state.xa_switch_configuration.size();
      EXPECT_TRUE( state.xa_switch_configuration.at( "db2").xa_struct_name == "db2xa_switch_static_std");
      EXPECT_TRUE( state.xa_switch_configuration.at( "rm-mockup").xa_struct_name == "casual_mockup_xa_switch_static");
   }

   TEST( casual_transaction_configuration, configure_resource__expect_2_resources)
   {
      CASUAL_UNITTEST_TRACE();

      local::Domain domain;

      transaction::State state( ":memory:");

      transaction::action::configure( state, "../example/resources/resources.yaml");

      ASSERT_TRUE( state.resources.size() == 2);
      EXPECT_TRUE( state.resources.at( 0).id == 1);
      EXPECT_TRUE( state.resources.at( 0).openinfo == "some open info 1");
      EXPECT_TRUE( state.resources.at( 1).id == 2);
      EXPECT_TRUE( state.resources.at( 1).closeinfo == "some close info 2");
   }

   /*
   TEST( casual_transaction_configuration, connect_to_broker__expect_running_message_sent)
   {
      common::mockup::queue::clearAllQueues();

      // prepare queues
      local::prepareConfigurationResponse();


      transaction::State state( "unittest_transaction.db");

      common::mockup::queue::blocking::Writer brokerQueue{ local::id::broker()};
      common::mockup::queue::blocking::Reader queueReader{ local::id::tm()};

      transaction::action::configure( state, brokerQueue, queueReader);


      common::mockup::queue::non_blocking::Reader brokerReplyQ{ local::id::broker()};
      common::message::transaction::Connected connected;

      ASSERT_TRUE( brokerReplyQ( connected));
      EXPECT_TRUE( connected.success);
   }
   */

} // casual


