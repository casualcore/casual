//!
//! test_configuration.cpp
//!
//! Created on: Aug 3, 2013
//!     Author: Lazan
//!


#include <gtest/gtest.h>


#include "transaction/manager/manager.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"

#include "common/message.h"
#include "common/mockup.h"
#include "common/internal/trace.h"
#include "common/internal/log.h"

namespace casual
{

   namespace local
   {
      namespace id
      {
         common::platform::queue_id_type broker() { return 10;}
         common::platform::queue_id_type tm() { return 20;}

      } // id

      common::file::ScopedPath transactionLogPath()
      {
         return common::file::ScopedPath{ "unittest_transaction_log.db"};
      }

      void prepareConfigurationResponse()
      {
         common::message::transaction::Configuration result;

         common::message::resource::Manager manager;
         manager.id = 1;
         manager.key = "rm-mockup";
         manager.instances = 3;
         manager.openinfo = "some open info 1";
         manager.closeinfo = "some close info 1";
         result.resources.push_back( manager);

         manager.id = 2;
         manager.key = "rm-mockup";
         manager.instances = 3;
         manager.openinfo = "some open info 2";
         manager.closeinfo = "some close info 2";
         result.resources.push_back( manager);

         common::mockup::queue::blocking::Writer tmWriter( local::id::tm());
         tmWriter( result);
      }




   } // local

   TEST( casual_transaction_configuration, configure_xa_config__expect_2_resources)
   {
      common::mockup::queue::clearAllQueues();

      // prepare queues
      local::prepareConfigurationResponse();


      auto path = local::transactionLogPath();
      transaction::State state( path);

      common::mockup::queue::blocking::Writer brokerQueue{ local::id::broker()};
      common::mockup::queue::blocking::Reader queueReader{ local::id::tm()};

      transaction::action::configure( state, brokerQueue, queueReader);

      ASSERT_TRUE( state.xaConfig.size() == 2);
      EXPECT_TRUE( state.xaConfig.at( "db2").xa_struct_name == "db2xa_switch_static_std");
      EXPECT_TRUE( state.xaConfig.at( "rm-mockup").xa_struct_name == "casual_mockup_xa_switch_static");
   }

   TEST( casual_transaction_configuration, configure_resource__expect_2_resources)
   {
      common::mockup::queue::clearAllQueues();

      // prepare queues
      local::prepareConfigurationResponse();


      auto path = local::transactionLogPath();
      transaction::State state( path);

      common::mockup::queue::blocking::Writer brokerQueue{ local::id::broker()};
      common::mockup::queue::blocking::Reader queueReader{ local::id::tm()};

      transaction::action::configure( state, brokerQueue, queueReader);

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


