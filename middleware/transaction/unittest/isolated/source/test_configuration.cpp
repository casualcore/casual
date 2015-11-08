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
         common::message::transaction::manager::Configuration configuration()
         {
            common::message::transaction::manager::Configuration result;

            common::message::transaction::resource::Manager manager;
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

            return result;
         }

         struct Domain
         {
            Domain()
             : broker{
               []( common::message::transaction::manager::connect::Request r)
               {
                  common::Trace trace{ "mockup transaction::manager::connect::Request", common::log::internal::debug};

                  auto reply = common::message::reverse::type( r);
                  reply.directive = decltype( reply)::Directive::start;

                  std::vector< common::mockup::reply::result_t> result;
                  result.emplace_back( r.process.queue, reply);

                  auto conf = configuration();

                  conf.correlation = r.correlation;
                  conf.domain = "mockup-domain";

                  result.emplace_back( r.process.queue, conf);

                  return result;
               }
            }
            {

            }


            common::mockup::domain::Broker broker;
         };

      } // <unnamed>
   } // local

   TEST( casual_transaction_configuration, configure_xa_config__expect_2_resources)
   {
      common::mockup::ipc::clear();

      local::Domain domain;

      transaction::State state( ":memory:");

      transaction::action::configure( state);

      ASSERT_TRUE( state.xaConfig.size() >= 2) << "state.xaConfig.size(): " << state.xaConfig.size();
      EXPECT_TRUE( state.xaConfig.at( "db2").xa_struct_name == "db2xa_switch_static_std");
      EXPECT_TRUE( state.xaConfig.at( "rm-mockup").xa_struct_name == "casual_mockup_xa_switch_static");
   }

   TEST( casual_transaction_configuration, configure_resource__expect_2_resources)
   {
      common::mockup::ipc::clear();

      local::Domain domain;

      transaction::State state( ":memory:");

      transaction::action::configure( state);

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


