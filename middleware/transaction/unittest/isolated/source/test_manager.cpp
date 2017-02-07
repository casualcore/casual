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
                  : m_filename{ common::mockup::file::temporary::content( ".yaml", configuration)},
                    env{ m_filename},
                    m_process{ "./bin/casual-transaction-manager", { "-l", ":memory:"}
                  }
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

               struct env_t
               {
                  env_t( const std::string& file)
                  {
                     common::environment::variable::set( "CASUAL_RESOURCE_CONFIGURATION_FILE", file);
                  }
               } env;

               common::mockup::Process m_process;

            };

            struct Domain
            {
               Domain( const std::string& configuration)
                  : manager{ configure()},
                     tm{ configuration}
               {
                  configure();
               }

               common::mockup::domain::Manager manager;
               common::mockup::domain::Broker broker;
               Manager tm;

            private:

               static common::message::domain::configuration::Domain configure()
               {
                  common::message::domain::configuration::Domain domain;

                  using resource_type = common::message::domain::configuration::transaction::Resource;

                  domain.transaction.resources = {
                        {
                           []( resource_type& r)
                           {
                              r.name = "rm1";
                              r.key = "rm-mockup";
                              r.instances = 2;
                              r.openinfo = "openinfo1";
                           }
                        },
                        {
                           []( resource_type& r)
                           {
                              r.name = "rm2";
                              r.key = "rm-mockup";
                              r.instances = 2;
                              r.openinfo = "openinfo2";
                           }
                        }
                  };

                  return domain;
               }

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
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            local::Domain domain{ local::configuration()};
         });
      }


      TEST( casual_transaction_manager, begin_transaction)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};
             

         EXPECT_TRUE( tx_begin() == TX_OK);

         auto state = local::admin::call::state();

         EXPECT_TRUE( state.transactions.empty());

         EXPECT_TRUE( tx_commit() == TX_OK);

      }


      TEST( casual_transaction_manager, commit_transaction__expect_ok__no_resource_roundtrips)
      {
         common::unittest::Trace trace;

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
         common::unittest::Trace trace;

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
            message.resources = { 1};

            local::send::tm( message);
         }

         EXPECT_TRUE( tx_commit() == TX_OK);


         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_stats( state);
         auto& rm1 = proxies.at( 0);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == 1);
         EXPECT_TRUE( rm1.name == "rm1");
         EXPECT_TRUE( rm1.statistics.resource.invoked == 1);
      }

      TEST( casual_transaction_manager, begin_rollback_transaction__1_resources_involved__expect_one_phase_commit_optimization)
      {
         common::unittest::Trace trace;

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
            message.resources = { 1};

            local::send::tm( message);
         }

         EXPECT_TRUE( tx_rollback() == TX_OK);

         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_stats( state);
         auto& rm1 = proxies.at( 0);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == 1);
         EXPECT_TRUE( rm1.statistics.resource.invoked == 1);
      }



      TEST( casual_transaction_manager, begin_rollback_transaction__2_resources_involved__expect_XA_OK)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};

         EXPECT_TRUE( tx_begin() == TX_OK);

         // involved
         {
            common::message::transaction::resource::Involved message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.resources = { 1, 2};

            local::send::tm( message);
         }

         EXPECT_TRUE( tx_rollback() == TX_OK);

         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_stats( state);
         auto& rm1 = proxies.at( 0);
         auto& rm2 = proxies.at( 1);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == 1);
         EXPECT_TRUE( rm1.statistics.resource.invoked == 1);

         ASSERT_TRUE( rm2.instances.size() == 2);
         EXPECT_TRUE( rm2.id == 2);
         EXPECT_TRUE( rm2.statistics.resource.invoked == 1);
      }


      TEST( casual_transaction_manager, begin_commit_transaction__2_resources_involved__expect_two_phase_commit)
      {
         common::unittest::Trace trace;

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
            message.resources = { 1, 2};

            local::send::tm( message);
         }

         EXPECT_TRUE( tx_commit() == TX_OK);

         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_stats( state);
         auto& rm1 = proxies.at( 0);
         auto& rm2 = proxies.at( 1);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == 1);
         EXPECT_TRUE( rm1.statistics.resource.invoked == 2); // 1 prepare, 1 commit

         ASSERT_TRUE( rm2.instances.size() == 2);
         EXPECT_TRUE( rm2.id == 2);
         EXPECT_TRUE( rm2.statistics.resource.invoked == 2); // 1 prepare, 1 commit
      }



      TEST( casual_transaction_manager, begin_transaction__2_resource_involved__owner_dies__expect_rollback)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};

         EXPECT_TRUE( tx_begin() == TX_OK);


         // involved
         {
            common::message::transaction::resource::Involved message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.resources = { 1, 2};

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

      TEST( casual_transaction_manager, begin_transaction__1_remote_resurce_involved___expect_one_phase_optimization)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};

         mockup::ipc::Collector gateway;

         auto trid = common::transaction::ID::create();

         // gateway involved
         {
            common::message::transaction::resource::external::Involved message;
            message.trid = trid;
            message.process = gateway.process();

            local::send::tm( message);
         }

         // commit
         {
            common::message::transaction::commit::Request message;
            message.trid = trid;
            message.process = process::handle();

            local::send::tm( message);
         }

         // remote commit (one phase optimization)
         {
            common::message::transaction::resource::commit::Request message;

            communication::ipc::blocking::receive( gateway.output(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.flags == TMONEPHASE);

            auto reply = common::message::reverse::type( message);
            reply.resource = message.resource;
            reply.state = XA_OK;
            reply.trid = message.trid;

            local::send::tm( reply);

         }

         // commit reply
         {
            common::message::transaction::commit::Reply message;

            communication::ipc::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == TX_OK);

         }


      }



      TEST( casual_transaction_manager, begin_transaction__2_remote_resurce_involved___expect_remote_prepare_commit)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};

         mockup::ipc::Collector gateway1;
         mockup::ipc::Collector gateway2;

         auto trid = common::transaction::ID::create();

         // gateway involved
         {
            common::message::transaction::resource::external::Involved message;
            message.trid = trid;

            message.process = gateway1.process();
            local::send::tm( message);

            message.process = gateway2.process();
            local::send::tm( message);
         }

         // commit
         {
            common::message::transaction::commit::Request message;
            message.trid = trid;
            message.process = process::handle();

            local::send::tm( message);
         }


         // remote prepare
         {
            auto remote_prepare = [&]( mockup::ipc::Collector& gtw){

               common::message::transaction::resource::prepare::Request message;

               communication::ipc::blocking::receive( gtw.output(), message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == TMNOFLAGS);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = XA_OK;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_prepare( gateway1);
            remote_prepare( gateway2);
         };

         // commit prepare reply
         {
            common::message::transaction::commit::Reply message;

            communication::ipc::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.stage == common::message::transaction::commit::Reply::Stage::prepare);
            EXPECT_TRUE( message.state == TX_OK);
         }


         // remote commit
         {
            auto remote_commit = [&]( mockup::ipc::Collector& gtw){

               common::message::transaction::resource::commit::Request message;

               communication::ipc::blocking::receive( gtw.output(), message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == TMNOFLAGS);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = XA_OK;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_commit( gateway1);
            remote_commit( gateway2);
         };

         // commit reply
         {
            common::message::transaction::commit::Reply message;

            communication::ipc::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.stage == common::message::transaction::commit::Reply::Stage::commit);
            EXPECT_TRUE( message.state == TX_OK);
         }
      }

      TEST( casual_transaction_manager, begin_transaction__2_remote_resurce_involved_read_only___expect_remote_prepare__read_only_optimization)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};

         mockup::ipc::Collector gateway1;
         mockup::ipc::Collector gateway2;

         auto trid = common::transaction::ID::create();

         // gateway involved
         {
            common::message::transaction::resource::external::Involved message;
            message.trid = trid;

            message.process = gateway1.process();
            local::send::tm( message);

            message.process = gateway2.process();
            local::send::tm( message);
         }

         // commit
         {
            common::message::transaction::commit::Request message;
            message.trid = trid;
            message.process = process::handle();

            local::send::tm( message);
         }


         // remote prepare
         {
            auto remote_prepare = [&]( mockup::ipc::Collector& gtw){

               common::message::transaction::resource::prepare::Request message;

               communication::ipc::blocking::receive( gtw.output(), message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == TMNOFLAGS);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = XA_RDONLY;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_prepare( gateway1);
            remote_prepare( gateway2);
         };

         // commit reply
         {
            common::message::transaction::commit::Reply message;

            communication::ipc::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid) << "message: " << message;
            EXPECT_TRUE( message.state == XA_RDONLY) << "state: " << message.state;
         }
      }

      TEST( casual_transaction_manager, transaction_2_remote_resurce_involved__one_phase_commit_optimzation___expect_prepare_phase_commit_XA_OK)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};

         mockup::ipc::Collector gateway1;
         mockup::ipc::Collector gateway2;

         auto trid = common::transaction::ID::create();

         // gateway involved
         {
            common::message::transaction::resource::external::Involved message;
            message.trid = trid;

            message.process = gateway1.process();
            local::send::tm( message);

            message.process = gateway2.process();
            local::send::tm( message);
         }

         // one-phase-commit
         {
            common::message::transaction::resource::commit::Request message;
            message.trid = trid;
            message.process = process::handle();
            message.flags = TMONEPHASE;

            local::send::tm( message);
         }


         // remote prepare
         {
            auto remote_prepare = [&]( mockup::ipc::Collector& gtw){

               common::message::transaction::resource::prepare::Request message;

               communication::ipc::blocking::receive( gtw.output(), message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == TMNOFLAGS);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = XA_OK;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_prepare( gateway1);
            remote_prepare( gateway2);
         };

         // remote commit
         {
            auto remote_commit = [&]( mockup::ipc::Collector& gtw){

               common::message::transaction::resource::commit::Request message;

               communication::ipc::blocking::receive( gtw.output(), message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == TMNOFLAGS);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = XA_OK;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_commit( gateway1);
            remote_commit( gateway2);
         };

         // resource commit reply
         {
            common::message::transaction::resource::commit::Reply message;

            communication::ipc::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid) << "message: " << message;
            EXPECT_TRUE( message.state == XA_OK) << "state: " << message.state;
         }
      }



      TEST( casual_transaction_manager, transaction_2_remote_resurce_involved__one_phase_commit_optimzation__RM_fail__expect_rollback__commit_XA_RBOTHER)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::configuration()};

         mockup::ipc::Collector gateway1;
         mockup::ipc::Collector gateway2;

         auto trid = common::transaction::ID::create();

         // gateway involved
         {
            common::message::transaction::resource::external::Involved message;
            message.trid = trid;

            message.process = gateway1.process();
            local::send::tm( message);

            message.process = gateway2.process();
            local::send::tm( message);
         }

         // one-phase-commit
         {
            common::message::transaction::resource::commit::Request message;
            message.trid = trid;
            message.process = process::handle();
            message.flags = TMONEPHASE;

            local::send::tm( message);
         }


         // remote prepare
         {
            auto remote_prepare = [&]( mockup::ipc::Collector& gtw){

               common::message::transaction::resource::prepare::Request message;

               communication::ipc::blocking::receive( gtw.output(), message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == TMNOFLAGS);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = XAER_RMERR;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_prepare( gateway1);
            remote_prepare( gateway2);
         };

         // remote rollback
         {
            auto remote_commit = [&]( mockup::ipc::Collector& gtw){

               common::message::transaction::resource::rollback::Request message;

               communication::ipc::blocking::receive( gtw.output(), message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == TMNOFLAGS);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = XA_OK;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_commit( gateway1);
            remote_commit( gateway2);
         };

         // resource commit reply
         {
            common::message::transaction::resource::commit::Reply message;

            communication::ipc::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid) << "message: " << message;
            EXPECT_TRUE( message.state == XA_RBOTHER) << "state: " << message.state;
         }
      }

   } // transaction

} // casual
