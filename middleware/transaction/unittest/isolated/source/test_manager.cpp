//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"


#include "transaction/manager/handle.h"
#include "transaction/manager/manager.h"


#include "common/mockup/ipc.h"
#include "common/mockup/file.h"
#include "common/mockup/domain.h"
#include "common/mockup/rm.h"
#include "common/mockup/process.h"

#include "common/trace.h"
#include "common/message/dispatch.h"
#include "common/message/transaction.h"
#include "common/environment.h"
#include "common/transcode.h"

#include "sf/xatmi_call.h"
#include "sf/archive/log.h"

#include "tx.h"

#include <fstream>



#define CASUAL_DEBUG_EXCEPTION( statement) \
   try \
   { \
      statement \
   } catch( const casual::common::exception::base& e) { std::cerr << "exception: " << e << std::endl; throw; } \
   catch( const std::exception& e) { std::cerr << "exception: " << e.what() << std::endl; throw; }

namespace casual
{

   using namespace common;

   namespace transaction
   {


      namespace local
      {
         namespace
         {
            struct Manager
            {

               Manager( const std::string& configuration)
                  : m_filename{ common::mockup::file::temporary( ".yaml", configuration)},
                    m_process{ "./bin/casual-transaction-manager",
                     { "-c", m_filename,
                       "-l", ":memory:",
                     }}
               {
                  //
                  // We wait until tm is up
                  //
                  common::process::ping(
                        common::process::instance::fetch::handle(
                              common::process::instance::identity::transaction::manager()).queue);
               }

            private:
               common::file::scoped::Path m_filename;
               common::mockup::Process m_process;

            };

            struct Domain
            {
               Domain( const std::string& configuration)
                  : manager
                    {
                        handle_resource_configuration{}
                     },
                    tm{ configuration}
               {

               }

               common::mockup::domain::Manager manager;
               common::mockup::domain::Broker broker;
               Manager tm;

            private:

               struct handle_resource_configuration
               {

                  void operator () ( common::message::domain::configuration::transaction::resource::Request& request) const
                  {
                     auto reply = common::message::reverse::type( request);

                     reply.resources.emplace_back(
                           []( common::message::domain::configuration::transaction::Resource& m)
                           {
                              m.id = 10;
                              m.key = "rm-mockup";
                              m.instances = 2;
                              m.openinfo = "rm-10";
                           });
                     reply.resources.emplace_back(
                           []( common::message::domain::configuration::transaction::Resource& m)
                           {
                              m.id = 11;
                              m.key = "rm-mockup";
                              m.instances = 2;
                              m.openinfo = "rm-11";
                           });

                     common::mockup::ipc::eventually::send( request.process.queue, reply);
                  }
               };

            };

            std::string configuration()
            {
               return R"(

   resources:
            
     - key: rm-mockup   
       server: "./bin/rm-proxy-casual-mockup"
       xa_struct_name: casual_mockup_xa_switch_static
       libraries:
         - casual-mockup-rm
   )";
            }


            namespace admin
            {
               namespace call
               {
                  vo::State state()
                  {
                     sf::xatmi::service::binary::Sync service( ".casual.transaction.state");
                     auto reply = service();

                     vo::State serviceReply;

                     reply >> CASUAL_MAKE_NVP( serviceReply);

                     return serviceReply;
                  }

               } // call

            } // admin

            namespace send
            {

               template< typename M>
               void tm( M&& message)
               {
                  communication::ipc::blocking::send(
                        common::communication::ipc::transaction::manager::device(), message);
               }

            } // send

            std::vector< vo::resource::Proxy> accumulate_stats( const vo::State& state)
            {
               auto result = state.resources;

               for( auto& proxy : result)
               {
                  for( auto& instance : proxy.instances)
                  {
                     proxy.statistics += instance.statistics;
                  }
               }
               return result;
            }

         } // <unnamed>
      } // local


      TEST( casual_transaction_manager, shutdown)
      {
         CASUAL_UNITTEST_TRACE();

         EXPECT_NO_THROW({
            local::Domain domain{ local::configuration()};
         });
      }


      TEST( casual_transaction_manager, begin_transaction)
      {
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::configuration()};
             

         EXPECT_TRUE( tx_begin() == TX_OK);

         auto state = local::admin::call::state();

         EXPECT_TRUE( state.transactions.empty());

         EXPECT_TRUE( tx_commit() == TX_OK);

      }


      TEST( casual_transaction_manager, commit_transaction__expect_ok__no_resource_roundtrips)
      {
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::configuration()};


         EXPECT_TRUE( tx_begin() == TX_OK);

         EXPECT_TRUE( tx_commit() == TX_OK);


         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         for( auto& resource : state.resources)
         {
            for( auto& instance : resource.instances)
            {
               EXPECT_TRUE( instance.statistics.resource.invoked == 0);
            }
         }
      }



      TEST( casual_transaction_manager, begin_commit_transaction__1_resources_involved__expect_one_phase_commit_optimization)
      {
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::configuration()};


         EXPECT_TRUE( tx_begin() == TX_OK);

         //
         // Make sure we make the transaction distributed
         //
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // involved
         {
            common::message::transaction::resource::Involved message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.resources = { 10};

            local::send::tm( message);
         }

         EXPECT_TRUE( tx_commit() == TX_OK);


         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_stats( state);
         auto& rm_10 = proxies.at( 0);

         ASSERT_TRUE( rm_10.instances.size() == 2);
         EXPECT_TRUE( rm_10.id == 10);
         EXPECT_TRUE( rm_10.statistics.resource.invoked == 1);
      }

      TEST( casual_transaction_manager, begin_rollback_transaction__1_resources_involved__expect_one_phase_commit_optimization)
      {
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::configuration()};

         EXPECT_TRUE( tx_begin() == TX_OK);

         //
         // Make sure we make the transaction distributed
         //
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // involved
         {
            common::message::transaction::resource::Involved message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.resources = { 10};

            local::send::tm( message);
         }

         EXPECT_TRUE( tx_rollback() == TX_OK);

         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_stats( state);
         auto& rm_10 = proxies.at( 0);

         ASSERT_TRUE( rm_10.instances.size() == 2);
         EXPECT_TRUE( rm_10.id == 10);
         EXPECT_TRUE( rm_10.statistics.resource.invoked == 1);
      }



      TEST( casual_transaction_manager, begin_rollback_transaction__2_resources_involved__expect_XA_OK)
      {
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::configuration()};

         EXPECT_TRUE( tx_begin() == TX_OK);

         // involved
         {
            common::message::transaction::resource::Involved message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.resources = { 10, 11};

            local::send::tm( message);
         }

         EXPECT_TRUE( tx_rollback() == TX_OK);

         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_stats( state);
         auto& rm_10 = proxies.at( 0);
         auto& rm_11 = proxies.at( 1);

         ASSERT_TRUE( rm_10.instances.size() == 2);
         EXPECT_TRUE( rm_10.id == 10);
         EXPECT_TRUE( rm_10.statistics.resource.invoked == 1);

         ASSERT_TRUE( rm_11.instances.size() == 2);
         EXPECT_TRUE( rm_11.id == 11);
         EXPECT_TRUE( rm_11.statistics.resource.invoked == 1);
      }


      TEST( casual_transaction_manager, begin_commit_transaction__2_resources_involved__expect_two_phase_commit)
      {
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::configuration()};


         EXPECT_TRUE( tx_begin() == TX_OK);

         //
         // Make sure we make the transaction distributed
         //
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // involved
         {
            common::message::transaction::resource::Involved message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.resources = { 10, 11};

            local::send::tm( message);
         }

         EXPECT_TRUE( tx_commit() == TX_OK);

         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_stats( state);
         auto& rm_10 = proxies.at( 0);
         auto& rm_11 = proxies.at( 1);

         ASSERT_TRUE( rm_10.instances.size() == 2);
         EXPECT_TRUE( rm_10.id == 10);
         EXPECT_TRUE( rm_10.statistics.resource.invoked == 2); // 1 prepare, 1 commit

         ASSERT_TRUE( rm_11.instances.size() == 2);
         EXPECT_TRUE( rm_11.id == 11);
         EXPECT_TRUE( rm_11.statistics.resource.invoked == 2); // 1 prepare, 1 commit
      }



      TEST( casual_transaction_manager, begin_transaction__2_resource_involved__owner_dies__expect_rollback)
      {
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::configuration()};

         EXPECT_TRUE( tx_begin() == TX_OK);


         // involved
         {
            common::message::transaction::resource::Involved message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.resources = { 10, 11};

            local::send::tm( message);
         }

         // caller dies
         {
            common::message::domain::process::termination::Event event;
            event.death.pid = process::handle().pid;
            event.death.reason = common::process::lifetime::Exit::Reason::core;

            local::send::tm( event);
         }


         // should be more than enough for TM to complete the rollback.
         process::sleep( std::chrono::milliseconds{ 10});


         auto state = local::admin::call::state();

         //
         // transaction should be rolled back and removed
         EXPECT_TRUE( state.transactions.empty());



         EXPECT_TRUE( tx_rollback() == TX_OK);
      }


   } // transaction

} // casual
