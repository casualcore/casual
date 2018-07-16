//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


//
// to be able to use 'raw' flags and codes
// since we undefine 'all' of them in common
//
#define CASUAL_NO_XATMI_UNDEFINE



#include <gtest/gtest.h>
#include "common/unittest.h"


#include "transaction/manager/handle.h"
#include "transaction/manager/manager.h"
#include "transaction/manager/admin/server.h"


#include "common/mockup/ipc.h"
#include "common/mockup/file.h"
#include "common/mockup/domain.h"
#include "common/mockup/rm.h"
#include "common/mockup/process.h"

#include "common/message/dispatch.h"
#include "common/message/transaction.h"
#include "common/environment.h"
#include "common/transcode.h"
#include "common/functional.h"

#include "common/communication/instance.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/archive/log.h"

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
                  common::communication::instance::ping(
                        common::communication::instance::fetch::handle(
                              common::communication::instance::identity::transaction::manager).ipc);
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
               Domain( common::message::domain::configuration::Domain configuration, const std::string& resource_configuration)
                  : manager{ std::move( configuration)},
                     tm{ resource_configuration}
               {
               }

               Domain()
                  : manager{ configuration()},
                     tm{ resource_configuration()}
               {
               }


               common::mockup::domain::Manager manager;
               common::mockup::domain::service::Manager service;
               Manager tm;

               static common::message::domain::configuration::Domain configuration()
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
                              //r.openinfo = "openinfo1";
                           }
                        },
                        {
                           []( resource_type& r)
                           {
                              r.name = "rm2";
                              r.key = "rm-mockup";
                              r.instances = 2;
                              //r.openinfo = "openinfo2";
                           }
                        }
                  };

                  return domain;
               }

               static std::string resource_configuration()
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

            };




            namespace admin
            {
               namespace call
               {
                  vo::State state()
                  {
                     serviceframework::service::protocol::binary::Call call;
                     auto reply = call( manager::admin::service::name::state());

                     vo::State result;
                     reply >> CASUAL_MAKE_NVP( result);

                     return result;
                  }

               } // call

            } // admin

            namespace send
            {

               template< typename M>
               void tm( M&& message)
               {
                  communication::ipc::blocking::send(
                        common::communication::instance::outbound::transaction::manager::device(), message);
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

            common::strong::resource::id rm_1{ 1};
            common::strong::resource::id rm_2{ 2};

         } // <unnamed>
      } // local


      TEST( casual_transaction_manager, shutdown)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            local::Domain domain;
         });
      }

      TEST( casual_transaction_manager, one_non_existent_RM_key__expect_boot)
      {
         common::unittest::Trace trace;

         auto configuration = local::Domain::configuration();
         configuration.transaction.resources.at( 1).key = "non-existent";


         EXPECT_NO_THROW({
            local::Domain domain( std::move( configuration), local::Domain::resource_configuration());
         });
      }

      TEST( casual_transaction_manager, non_existent_RM_proxy___expect_boot)
      {
         common::unittest::Trace trace;

         auto rm_properties = R"(
resources:

  - key: rm-mockup   
    server: "./non/existent/path"
    xa_struct_name: casual_mockup_xa_switch_static
    libraries:
       - casual-mockup-rm
)";


         EXPECT_NO_THROW({
            local::Domain domain( local::Domain::configuration(), rm_properties);
         });
      }


      TEST( casual_transaction_manager, one_RM_xa_open__error___expect_boot)
      {
         common::unittest::Trace trace;

         auto configuration = local::Domain::configuration();
         configuration.transaction.resources.at( 1).openinfo = "--open " + std::to_string( XAER_RMFAIL);


         EXPECT_NO_THROW({
            local::Domain domain( std::move( configuration), local::Domain::resource_configuration());
         });
      }


      TEST( casual_transaction_manager, resource_lookup_request)
      {
         common::unittest::Trace trace;

         auto configuration = local::Domain::configuration();
         configuration.transaction.resources.at( 1).openinfo = "openinfo2";

         local::Domain domain( std::move( configuration), local::Domain::resource_configuration());

         common::message::transaction::resource::lookup::Request request;
         request.process = common::process::handle();
         request.resources = { "rm2"};

         auto reply = common::communication::ipc::call( common::communication::instance::outbound::transaction::manager::device(), request);

         ASSERT_TRUE( reply.resources.size() == 1);
         EXPECT_TRUE( reply.resources.at( 0).name == "rm2");
         EXPECT_TRUE( reply.resources.at( 0).openinfo == "openinfo2");
      }


      TEST( casual_transaction_manager, begin_transaction)
      {
         common::unittest::Trace trace;

         local::Domain domain;
             

         EXPECT_TRUE( tx_begin() == TX_OK);

         auto state = local::admin::call::state();

         EXPECT_TRUE( state.transactions.empty());
         EXPECT_TRUE( tx_commit() == TX_OK);
      }


      TEST( casual_transaction_manager, commit_transaction__expect_ok__no_resource_roundtrips)
      {
         common::unittest::Trace trace;

         local::Domain domain;


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

         local::Domain domain;


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
            message.resources = { local::rm_1};

            local::send::tm( message);
         }

         EXPECT_TRUE( tx_commit() == TX_OK);


         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_stats( state);
         auto& rm1 = proxies.at( 0);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == local::rm_1);
         EXPECT_TRUE( rm1.name == "rm1");
         EXPECT_TRUE( rm1.statistics.resource.invoked == 1);
      }


      namespace local
      {
         namespace
         {
            template< typename F, typename... Args>
            common::code::tx tx_invoke( F function, Args&&... args)
            {
               return static_cast< common::code::tx>( common::invoke( function, std::forward< Args>( args)...));
            }
         } // <unnamed>
      } // local

      TEST( casual_transaction_manager, begin_commit_transaction__1_resources_involved__xa_XA_RBDEADLOCK___expect__TX_HAZARD)
      {
         common::unittest::Trace trace;

         auto configuration = local::Domain::configuration();
         configuration.transaction.resources.resize( 1);
         configuration.transaction.resources.at( 0).openinfo = "--commit " + std::to_string( XA_RBDEADLOCK);

         local::Domain domain( std::move( configuration), local::Domain::resource_configuration());


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
            message.resources = { local::rm_1};

            local::send::tm( message);
         }

         using tx = common::code::tx;
         auto result = local::tx_invoke( &tx_commit);
         ASSERT_TRUE( result == tx::hazard) << "result: " << result;

         EXPECT_TRUE( local::tx_invoke( &tx_rollback) == tx::ok);

      }


      TEST( casual_transaction_manager, begin_commit_transaction__1_resources_involved__XAER_NOTA___expect__TX_OK)
      {
         common::unittest::Trace trace;

         auto configuration = local::Domain::configuration();
         configuration.transaction.resources.resize( 1);
         configuration.transaction.resources.at( 0).openinfo = "--commit " + std::to_string( XAER_NOTA);

         local::Domain domain( std::move( configuration), local::Domain::resource_configuration());

         using tx = common::code::tx;

         EXPECT_TRUE( local::tx_invoke( &tx_begin) == tx::ok);

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
            message.resources = { local::rm_1};

            local::send::tm( message);
         }

         auto result = local::tx_invoke( &tx_commit);
         EXPECT_TRUE( result == tx::ok) << "result: " << result;
      }

      TEST( casual_transaction_manager, begin_commit_transaction__1_resources_involved__environment_open_info__XAER_NOTA___expect__TX_OK)
      {
         common::unittest::Trace trace;

         common::environment::variable::set( "CASUAL_OPEN_INFO", "--commit " + std::to_string( XAER_NOTA));

         auto configuration = local::Domain::configuration();
         configuration.transaction.resources.resize( 1);
         configuration.transaction.resources.at( 0).openinfo = "${CASUAL_OPEN_INFO}";

         local::Domain domain( std::move( configuration), local::Domain::resource_configuration());

         using tx = common::code::tx;

         EXPECT_TRUE( local::tx_invoke( &tx_begin) == tx::ok);

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
            message.resources = { local::rm_1};

            local::send::tm( message);
         }

         auto result = local::tx_invoke( &tx_commit);
         EXPECT_TRUE( result == tx::ok) << "result: " << result;
      }

      TEST( casual_transaction_manager, begin_rollback_transaction__1_resources_involved__expect_one_phase_commit_optimization)
      {
         common::unittest::Trace trace;

         local::Domain domain;

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
            message.resources = { local::rm_1};

            local::send::tm( message);
         }

         EXPECT_TRUE( tx_rollback() == TX_OK);

         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_stats( state);
         auto& rm1 = proxies.at( 0);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == local::rm_1);
         EXPECT_TRUE( rm1.statistics.resource.invoked == 1);
      }



      TEST( casual_transaction_manager, begin_rollback_transaction__2_resources_involved__expect_XA_OK)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( tx_begin() == TX_OK);

         // involved
         {
            common::message::transaction::resource::Involved message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.resources = { local::rm_1, local::rm_2};

            local::send::tm( message);
         }

         EXPECT_TRUE( tx_rollback() == TX_OK);

         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_stats( state);
         auto& rm1 = proxies.at( 0);
         auto& rm2 = proxies.at( 1);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == local::rm_1);
         EXPECT_TRUE( rm1.statistics.resource.invoked == 1);

         ASSERT_TRUE( rm2.instances.size() == 2);
         EXPECT_TRUE( rm2.id == local::rm_2);
         EXPECT_TRUE( rm2.statistics.resource.invoked == 1);
      }


      TEST( casual_transaction_manager, begin_commit_transaction__2_resources_involved__expect_two_phase_commit)
      {
         common::unittest::Trace trace;

         local::Domain domain;


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
            message.resources = { local::rm_1, local::rm_2};

            local::send::tm( message);
         }

         EXPECT_TRUE( tx_commit() == TX_OK);

         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_stats( state);
         auto& rm1 = proxies.at( 0);
         auto& rm2 = proxies.at( 1);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == local::rm_1);
         EXPECT_TRUE( rm1.statistics.resource.invoked == 2); // 1 prepare, 1 commit

         ASSERT_TRUE( rm2.instances.size() == 2);
         EXPECT_TRUE( rm2.id == local::rm_2);
         EXPECT_TRUE( rm2.statistics.resource.invoked == 2); // 1 prepare, 1 commit
      }



      TEST( casual_transaction_manager, begin_transaction__2_resource_involved__owner_dies__expect_rollback)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( tx_begin() == TX_OK);


         // involved
         {
            common::message::transaction::resource::Involved message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.resources = { local::rm_1, local::rm_2};

            local::send::tm( message);
         }

         // caller dies
         {
            common::message::event::process::Exit event;
            event.state.pid = process::handle().pid;
            event.state.reason = common::process::lifetime::Exit::Reason::core;

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

      TEST( casual_transaction_manager, remote_resource_commit_one_phase__xid_unknown___expect_read_only)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto trid = common::transaction::ID::create();


         // remote commit
         {
            common::message::transaction::resource::commit::Request message;
            message.trid = trid;
            message.process = process::handle();
            message.flags = common::flag::xa::Flag::one_phase;

            local::send::tm( message);
         }

         // commit reply
         {
            common::message::transaction::resource::commit::Reply message;

            communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::read_only);
         }
      }

      TEST( casual_transaction_manager, remote_resource_prepare__xid_unknown___expect_read_only)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto trid = common::transaction::ID::create();


         // remote commit
         {
            common::message::transaction::resource::prepare::Request message;
            message.trid = trid;
            message.process = process::handle();
         
            local::send::tm( message);
         }

         // commit reply
         {
            common::message::transaction::resource::prepare::Reply message;

            communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::read_only);
         }
      }

      TEST( casual_transaction_manager, remote_resource_rollback__xid_unknown___expect_read_only)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto trid = common::transaction::ID::create();


         // remote rollback
         {
            common::message::transaction::resource::rollback::Request message;
            message.trid = trid;
            message.process = process::handle();
         
            local::send::tm( message);
         }

         // commit reply
         {
            common::message::transaction::resource::rollback::Reply message;

            communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::read_only);
         }
      }


      TEST( casual_transaction_manager, remote_owner__local_resource_involved__remote_rollback____expect_rollback)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto trid = common::transaction::ID::create();


         // local involved
         {
            common::message::transaction::resource::Involved message;
            message.trid = trid;
            message.process = process::handle();
            message.resources = { local::rm_1};

            local::send::tm( message);
         }

         // remote rollback
         {
            common::message::transaction::resource::rollback::Request message;
            message.trid = trid;
            message.process = process::handle();

            local::send::tm( message);
         }

         // rollback reply
         {
            common::message::transaction::resource::rollback::Reply message;

            communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::ok) << "state: " << message.state;
         }
      }

      TEST( casual_transaction_manager, remote_owner__same_remote_resource_involved__remote_rollback____expect_xa_read_only)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto trid = common::transaction::ID::create();


         // remote involved
         {
            common::message::transaction::resource::external::Involved message;
            message.trid = trid;
            message.process = process::handle();

            local::send::tm( message);
         }

         // remote rollback
         {
            common::message::transaction::resource::rollback::Request message;
            message.trid = trid;
            message.process = process::handle();

            local::send::tm( message);
         }

         // rollback reply
         {
            common::message::transaction::resource::rollback::Reply message;

            communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::read_only) << "state: " << message.state;
         }
      }


      TEST( casual_transaction_manager, begin_transaction__1_remote_resource_involved___expect_one_phase_optimization)
      {
         common::unittest::Trace trace;

         local::Domain domain;

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
            EXPECT_TRUE( message.flags == common::flag::xa::Flag::one_phase);

            auto reply = common::message::reverse::type( message);
            reply.resource = message.resource;
            reply.state = common::code::xa::ok;
            reply.trid = message.trid;

            local::send::tm( reply);

         }

         // commit reply
         {
            common::message::transaction::commit::Reply message;

            communication::ipc::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::tx::ok);

         }
      }



      TEST( casual_transaction_manager, begin_transaction__2_remote_resource_involved___expect_remote_prepare_commit)
      {
         common::unittest::Trace trace;

         local::Domain domain;

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
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::ok;
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
            EXPECT_TRUE( message.state == common::code::tx::ok);
         }


         // remote commit
         {
            auto remote_commit = [&]( mockup::ipc::Collector& gtw){

               common::message::transaction::resource::commit::Request message;

               communication::ipc::blocking::receive( gtw.output(), message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::ok;
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
            EXPECT_TRUE( message.state == common::code::tx::ok);
         }
      }

      TEST( casual_transaction_manager, begin_transaction__2_remote_resource_involved_read_only___expect_remote_prepare__read_only_optimization)
      {
         common::unittest::Trace trace;

         local::Domain domain;

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
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::read_only;
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
            //EXPECT_TRUE( message.state == XA_RDONLY) << "state: " << message.state;
            EXPECT_TRUE( message.state == common::code::tx::ok) << "state: " << message.state;
         }
      }

      TEST( casual_transaction_manager, transaction_2_remote_resource_involved__one_phase_commit_optimization___expect_prepare_phase_commit_XA_OK)
      {
         common::unittest::Trace trace;

         local::Domain domain;

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
            message.flags = common::flag::xa::Flag::one_phase;

            local::send::tm( message);
         }


         // remote prepare
         {
            auto remote_prepare = [&]( mockup::ipc::Collector& gtw){

               common::message::transaction::resource::prepare::Request message;

               communication::ipc::blocking::receive( gtw.output(), message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::ok;
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
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::ok;
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
            EXPECT_TRUE( message.state == common::code::xa::ok) << "state: " << message.state;
         }
      }



      TEST( casual_transaction_manager, transaction_2_remote_resource_involved__one_phase_commit_optimization__RM_fail__expect_rollback__commit_XA_RBOTHER)
      {
         common::unittest::Trace trace;

         local::Domain domain;

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
            message.flags = common::flag::xa::Flag::one_phase;

            local::send::tm( message);
         }


         // remote prepare
         {
            auto remote_prepare = [&]( mockup::ipc::Collector& gtw){

               common::message::transaction::resource::prepare::Request message;

               communication::ipc::blocking::receive( gtw.output(), message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::resource_error;
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
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::ok;
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
            EXPECT_TRUE( message.state == common::code::xa::rollback_other) << "state: " << message.state;
         }
      }

      namespace local
      {
         namespace
         {
            /*
            common::transaction::Resource xa_resource( std::string openinfo)
            {
               common::transaction::Resource result{ "rm-mockup", &casual_mockup_xa_switch_static};

               result.id = common::strong::resource::id{ 1};
               result.openinfo = std::move( openinfo);

               return result;
            }
            */

         } // <unnamed>
      } // local

      TEST( casual_transaction_manager, one_local_resource__configure)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_NO_THROW({
            common::transaction::context().configure( {}, { "rm1"});
         });
      }


      TEST( casual_transaction_manager, one_local_resource__begin_commit)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         ASSERT_NO_THROW({
            //common::transaction::context().configure( { local::xa_resource( "")}, {});
            common::transaction::context().configure( {}, { "rm1"});
         });

         common::transaction::context().begin();

         ASSERT_NO_THROW({
            common::transaction::context().commit();
         });
      }


   } // transaction

} // casual
