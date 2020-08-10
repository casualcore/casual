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


#include "common/unittest.h"

#include "transaction/manager/handle.h"
#include "transaction/manager/manager.h"
#include "transaction/manager/admin/server.h"
#include "transaction/manager/admin/transform.h"

#include "common/message/dispatch.h"
#include "common/message/transaction.h"
#include "common/environment.h"
#include "common/transcode.h"
#include "common/functional.h"

#include "common/communication/instance.h"

#include "common/unittest/file.h"
#include "domain/manager/unittest/process.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/log.h"

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
            
            namespace environment
            {
               constexpr auto rm1 = "CASUAL_UNITTEST_OPEN_INFO_RM1";
               constexpr auto rm2 = "CASUAL_UNITTEST_OPEN_INFO_RM2";
            } // environment
            
            struct Domain
            {
               struct Configuration
               {
                  std::string domain =  R"(
domain:
   name: transaction-domain

   groups:
      - name: first
      - name: second
        dependencies: [ first]

   transaction:
      resources:
         - key: rm-mockup
           name: rm1
           instances: 2
           openinfo: "${CASUAL_UNITTEST_OPEN_INFO_RM1}"
         - key: rm-mockup
           name: rm2
           instances: 2
           openinfo: "${CASUAL_UNITTEST_OPEN_INFO_RM2}"

   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
        memberships: [ first]
      - path: "./bin/casual-transaction-manager"
        arguments: [ --transaction-log, ":memory:"]
        memberships: [ second]
         
)";
                  std::string resource = R"(
resources:
  - key: rm-mockup
    server: "./bin/rm-proxy-casual-mockup"
    xa_struct_name: casual_mockup_xa_switch_static
    libraries:
      - casual-mockup-rm
)";

               };

               Domain( Configuration configuration) 
                  : environment{ configuration.resource}, 
                     process{ { std::move( configuration.domain)}} {}
               Domain() : Domain{ Configuration{}} {}
               
               struct Environment 
               {
                  Environment( const std::string& configuration) 
                     : resource{ common::unittest::file::temporary::content( ".yaml", configuration)}
                  {
                     common::environment::variable::set( "CASUAL_RESOURCE_CONFIGURATION_FILE", resource);

                     if( ! common::environment::variable::exists( environment::rm1))
                        common::environment::variable::set( environment::rm1, {});

                     if( ! common::environment::variable::exists( environment::rm2))
                        common::environment::variable::set( environment::rm2, {});
                  }

                  ~Environment() 
                  {
                     common::environment::variable::unset( environment::rm1);
                     common::environment::variable::unset( environment::rm2);
                  }
                  
                  common::file::scoped::Path resource;
               } environment;

               casual::domain::manager::unittest::Process process;
            };

            namespace admin
            {
               namespace call
               {
                  manager::admin::model::State state()
                  {
                     serviceframework::service::protocol::binary::Call call;
                     auto reply = call( manager::admin::service::name::state());

                     manager::admin::model::State result;
                     reply >> CASUAL_NAMED_VALUE( result);

                     return result;
                  }

               } // call

            } // admin

            namespace send
            {
               template< typename M>
               void tm( M&& message)
               {
                  communication::device::blocking::send(
                        common::communication::instance::outbound::transaction::manager::device(), message);
               }
            } // send

            namespace call
            {
               template< typename M>
               auto tm( M&& message)
               {
                  return communication::ipc::call(
                        common::communication::instance::outbound::transaction::manager::device(), message);
               }
            } // send

            std::vector< manager::admin::model::resource::Proxy> accumulate_metrics( const manager::admin::model::State& state)
            {
               auto result = state.resources;

               for( auto& proxy : result)
               {
                  auto metric_p = manager::admin::transform::metrics( proxy.metrics);
                  for( auto& instance : proxy.instances)
                  {
                     auto metric_i = manager::admin::transform::metrics( instance.metrics);
                     metric_p.resource += metric_i.resource;
                     metric_p.roundtrip += metric_i.roundtrip; 
                  }
                  proxy.metrics = manager::admin::transform::metrics( metric_p);
               }
               return result;
            }

            common::strong::resource::id rm_1{ 1};
            common::strong::resource::id rm_2{ 2};

            template< typename A>
            std::error_code wrap( A&& action)
            {
               try
               {
                  action();
                  return common::code::tx::ok;
               }
               catch( ...)
               {
                  return exception::code();
               }
            }

            auto begin() 
            {
               return wrap( [](){ common::transaction::context().begin();});
            }

            auto commit() 
            {
               return wrap( [](){ common::transaction::context().commit();});
            }

            auto rollback() 
            {
               return wrap( [](){ common::transaction::context().rollback();});
            }

         } // <unnamed>
      } // local


      TEST( transaction_manager, shutdown)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            local::Domain domain;
         });
      }

      TEST( transaction_manager, one_non_existent_RM_key__expect_boot)
      {
         common::unittest::Trace trace;

         auto configuration = []()
         {
            local::Domain::Configuration result;
            result.domain = R"(
domain:
   name: transaction-domain
   
   groups:
      - name: first
      - name: second
        dependencies: [ first]
   
   transaction:
      resources:
         - key: non-existent
           name: rm1
           instances: 2

   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
        memberships: [ first]
      - path: "./bin/casual-transaction-manager"
        arguments: [ --transaction-log, ":memory:"]
        memberships: [ second]
         
)";
            return result;
         }();


         EXPECT_NO_THROW({
            local::Domain domain( std::move( configuration));
         });
      }

      TEST( transaction_manager, non_existent_RM_proxy___expect_boot)
      {
         common::unittest::Trace trace;

         auto configuration = []()
         {
            local::Domain::Configuration result;
            result.resource =  R"(
resources:

  - key: rm-mockup   
    server: "./non/existent/path"
    xa_struct_name: casual_mockup_xa_switch_static
    libraries:
       - casual-mockup-rm
)";
            return result;
         }();


         EXPECT_NO_THROW({
            local::Domain domain( std::move( configuration));
         });
      }


      TEST( transaction_manager, one_RM_xa_open__error___expect_boot)
      {
         common::unittest::Trace trace;

         // we set unittest environment variable to set "error"
         common::environment::variable::set( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            "--open " + std::to_string( XAER_RMFAIL));

         EXPECT_NO_THROW({
            local::Domain domain;
         });
      }


      TEST( transaction_manager, resource_lookup_request)
      {
         common::unittest::Trace trace;

         auto configuration = []()
         {
            local::Domain::Configuration result;
            result.domain = R"(
domain:
   name: transaction-domain

   groups:
      - name: first
      - name: second
        dependencies: [ first]

   transaction:
      resources:
         - key: rm-mockup
           name: rm1
           instances: 2
         - key: rm-mockup
           name: rm2
           instances: 2
           openinfo: openinfo2

   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
        memberships: [ first]
      - path: "./bin/casual-transaction-manager"
        arguments: [ --transaction-log, ":memory:"]
        memberships: [ second]
         
)";
            return result;
         }();

         local::Domain domain( std::move( configuration));

         common::message::transaction::resource::lookup::Request request;
         request.process = common::process::handle();
         request.resources = { "rm2"};

         auto reply = common::communication::ipc::call( common::communication::instance::outbound::transaction::manager::device(), request);

         ASSERT_TRUE( reply.resources.size() == 1);
         EXPECT_TRUE( reply.resources.at( 0).name == "rm2");
         EXPECT_TRUE( reply.resources.at( 0).openinfo == "openinfo2");
      }


      TEST( transaction_manager, begin_transaction)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         auto state = local::admin::call::state();

         EXPECT_TRUE( state.transactions.empty()) << "state.transactions: " << state.transactions;

         EXPECT_TRUE( local::commit() == common::code::tx::ok);
      }


      TEST( transaction_manager, commit_transaction__expect_ok__no_resource_roundtrips)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( local::begin() == common::code::tx::ok);
         EXPECT_TRUE( local::commit() == common::code::tx::ok);

         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         for( auto& resource : state.resources)
         {
            for( auto& instance : resource.instances)
            {
               EXPECT_TRUE( instance.metrics.resource.count == 0);
            }
         }
      }


      TEST( transaction_manager, begin_commit_transaction__1_resources_involved__expect_one_phase_commit_optimization)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);

         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_metrics( state);
         auto& rm1 = proxies.at( 0);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == local::rm_1);
         EXPECT_TRUE( rm1.name == "rm1");
         EXPECT_TRUE( rm1.metrics.resource.count == 1) << CASUAL_NAMED_VALUE( rm1);
      }

      TEST( transaction_manager, begin_commit_transaction__1_resources_involved__2_times___expect_one_phase_commit_optimization)
      {
         common::unittest::Trace trace;

         local::Domain domain;


         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());



         // first time involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         // second time involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            ASSERT_TRUE( reply.involved.size() == 1);
            EXPECT_TRUE( reply.involved.at( 0) == local::rm_1);
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);

         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_metrics( state);
         auto& rm1 = proxies.at( 0);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == local::rm_1);
         EXPECT_TRUE( rm1.name == "rm1");
         EXPECT_TRUE( rm1.metrics.resource.count == 1) << CASUAL_NAMED_VALUE( rm1);
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

      TEST( transaction_manager, begin_commit_transaction__1_resources_involved__xa_XA_RBDEADLOCK___expect__TX_HAZARD)
      {
         common::unittest::Trace trace;

         // we set unittest environment variable to set "error"
         common::environment::variable::set( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            "--commit " + std::to_string( XA_RBDEADLOCK));

         local::Domain domain;

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         auto result = local::commit();
         ASSERT_TRUE( result == common::code::tx::hazard) << "result: " << result;

         EXPECT_TRUE( local::rollback() == common::code::tx::ok);
      }


      TEST( transaction_manager, no_transaction__1_resources_involved__XAER_NOTA___expect__TX_OK)
      {
         common::unittest::Trace trace;

         // we set unittest environment variable to set "error"
         common::environment::variable::set( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            "--commit " + std::to_string( XAER_NOTA));

         local::Domain domain;

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }
         EXPECT_TRUE( local::commit() == common::code::tx::ok);
      }

      TEST( transaction_manager, begin_commit_transaction__1_resources_involved__environment_open_info__XAER_NOTA___expect__TX_OK)
      {
         common::unittest::Trace trace;

         // we set unittest environment variable to set "error"
         common::environment::variable::set( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            "--commit " + std::to_string( XAER_NOTA));

         local::Domain domain;

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);
      }

      TEST( transaction_manager, begin_rollback_transaction__1_resources_involved__expect_XA_OK)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }     

         EXPECT_TRUE( local::rollback() == common::code::tx::ok);

         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_metrics( state);
         auto& rm1 = proxies.at( 0);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == local::rm_1);
         EXPECT_TRUE( rm1.metrics.resource.count == 1);
      }



      TEST( transaction_manager, begin_rollback_transaction__2_resources_involved__expect_XA_OK)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_1, local::rm_2};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_TRUE( local::rollback() == common::code::tx::ok);

         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty()) << CASUAL_NAMED_VALUE( state);

         auto proxies = local::accumulate_metrics( state);
         auto& rm1 = proxies.at( 0);
         auto& rm2 = proxies.at( 1);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == local::rm_1);
         EXPECT_TRUE( rm1.metrics.resource.count == 1) << CASUAL_NAMED_VALUE( rm1.metrics.resource);

         ASSERT_TRUE( rm2.instances.size() == 2);
         EXPECT_TRUE( rm2.id == local::rm_2);
         EXPECT_TRUE( rm2.metrics.resource.count == 1);
      }


      TEST( transaction_manager, begin_commit_transaction__2_resources_involved__expect_two_phase_commit)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // first rm involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         // second rm involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_2};
            
            auto reply = local::call::tm( message);

            //! should give the first rm as already involved
            ASSERT_TRUE( reply.involved.size() == 1);
            EXPECT_TRUE( reply.involved.at( 0) == local::rm_1);
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);

         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_metrics( state);
         auto& rm1 = proxies.at( 0);
         auto& rm2 = proxies.at( 1);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == local::rm_1);
         EXPECT_TRUE( rm1.metrics.resource.count == 2); // 1 prepare, 1 commit

         ASSERT_TRUE( rm2.instances.size() == 2);
         EXPECT_TRUE( rm2.id == local::rm_2);
         EXPECT_TRUE( rm2.metrics.resource.count == 2); // 1 prepare, 1 commit
      }



      TEST( transaction_manager, begin_transaction__2_resource_involved__owner_dies__expect_rollback)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_1, local::rm_2};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
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

         // transaction should be rolled back and removed
         EXPECT_TRUE( state.transactions.empty());

         EXPECT_TRUE( local::rollback() == common::code::tx::ok);
      }

      TEST( transaction_manager, remote_resource_commit_one_phase__xid_unknown___expect_read_only)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto trid = common::transaction::id::create();


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

            communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid) << "trid: " << trid << "\nmessage.trid: " << message.trid;
            EXPECT_TRUE( message.state == common::code::xa::read_only);
         }
      }

      TEST( transaction_manager, remote_resource_prepare__xid_unknown___expect_read_only)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto trid = common::transaction::id::create();


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

            communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::read_only);
         }
      }

      TEST( transaction_manager, remote_resource_rollback__xid_unknown___expect_xa_ok)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto trid = common::transaction::id::create();


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

            communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            // rollback has no optimization, just ok, or errors...
            EXPECT_TRUE( message.state == common::code::xa::ok);
         }
      }


      TEST( transaction_manager, remote_owner__local_resource_involved__remote_rollback____expect_rollback)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto trid = common::transaction::id::create();


         // local involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = trid;
            message.process = process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty()) << "reply.involved: " << reply.involved;
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

            communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::ok) << "state: " << message.state;
         }
      }

      TEST( transaction_manager, remote_owner__same_remote_resource_involved__remote_rollback____expect_xa_read_only)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto trid = common::transaction::id::create();


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

            communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::read_only) << "state: " << message.state;
         }
      }


      TEST( transaction_manager, begin_transaction__1_remote_resource_involved___expect_one_phase_optimization)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto trid = common::transaction::id::create();

         // external involved
         {
            common::message::transaction::resource::external::Involved message;
            message.trid = trid;
            message.process = process::handle();

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

            communication::device::blocking::receive( communication::ipc::inbound::device(), message);

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

            communication::device::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::tx::ok);

         }
      }

      namespace local
      {
         namespace
         {
            namespace involved
            {
               auto next()
               {
                  int global{};
                  return strong::process::id{ ++global};
               }

               struct Process 
               {
                  communication::ipc::inbound::Device inbound;
                  process::Handle process{ next(), inbound.connector().handle().ipc()};
               };
            } // involved
         } // <unnamed>
      } // local

      TEST( transaction_manager, begin_transaction__2_remote_resource_involved___expect_remote_prepare_commit)
      {
         common::unittest::Trace trace;

         local::Domain domain;


         local::involved::Process rm1;
         local::involved::Process rm2;


         auto trid = common::transaction::id::create();

         // gateway involved
         {
            common::message::transaction::resource::external::Involved message;
            message.trid = trid;

            message.process = rm1.process;
            local::send::tm( message);

            message.process = rm2.process;
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
            auto remote_prepare = [&]( auto& involved){

               common::message::transaction::resource::prepare::Request message;

               communication::device::blocking::receive( involved.inbound, message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::ok;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_prepare( rm1);
            remote_prepare( rm2);
         };

         // commit prepare reply
         {
            common::message::transaction::commit::Reply message;

            communication::device::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.stage == common::message::transaction::commit::Reply::Stage::prepare);
            EXPECT_TRUE( message.state == common::code::tx::ok);
         }


         // remote commit
         {
            auto remote_commit = [&]( auto& involved){

               common::message::transaction::resource::commit::Request message;

               communication::device::blocking::receive( involved.inbound, message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::ok;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_commit( rm1);
            remote_commit( rm2);
         };

         // commit reply
         {
            common::message::transaction::commit::Reply message;

            communication::device::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.stage == common::message::transaction::commit::Reply::Stage::commit);
            EXPECT_TRUE( message.state == common::code::tx::ok);
         }
      }

      TEST( transaction_manager, begin_transaction__2_remote_resource_involved_read_only___expect_remote_prepare__read_only_optimization)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         local::involved::Process rm1;
         local::involved::Process rm2;

         auto trid = common::transaction::id::create();

         // gateway involved
         {
            common::message::transaction::resource::external::Involved message;
            message.trid = trid;

            message.process = rm1.process;
            local::send::tm( message);

            message.process = rm2.process;
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
            auto remote_prepare = [&]( auto& involved){

               common::message::transaction::resource::prepare::Request message;

               communication::device::blocking::receive( involved.inbound, message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::read_only;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_prepare( rm1);
            remote_prepare( rm2);
         };

         // commit reply
         {
            common::message::transaction::commit::Reply message;

            communication::device::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.state == common::code::tx::ok) << CASUAL_NAMED_VALUE( message.state);
         }
      }


      TEST( transaction_manager, transaction_2_remote_resource_involved__one_phase_commit_optimization___expect_prepare_phase_commit_XA_OK)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         local::involved::Process rm1;
         local::involved::Process rm2;

         auto trid = common::transaction::id::create();

         // gateway involved
         {
            common::message::transaction::resource::external::Involved message;
            message.trid = trid;

            message.process = rm1.process;
            local::send::tm( message);

            message.process = rm2.process;
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
            auto remote_prepare = [&]( auto& involved)
            {
               common::message::transaction::resource::prepare::Request message;

               communication::device::blocking::receive( involved.inbound, message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::ok;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_prepare( rm1);
            remote_prepare( rm2);
         };

         // remote commit
         {
            auto remote_commit = [&]( auto& involved)
            {
               common::message::transaction::resource::commit::Request message;

               communication::device::blocking::receive( involved.inbound, message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::ok;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_commit( rm1);
            remote_commit( rm2);
         };

         // resource commit reply
         {
            common::message::transaction::resource::commit::Reply message;

            communication::device::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.state == common::code::xa::ok) << CASUAL_NAMED_VALUE( message.state);
         }
      }

      TEST( transaction_manager, transaction_2_remote_resource_involved__one_phase_commit_optimization__RM_fail__expect_rollback__commit_XA_RBOTHER)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         local::involved::Process rm1;
         local::involved::Process rm2;

         auto trid = common::transaction::id::create();

         // gateway involved
         {
            common::message::transaction::resource::external::Involved message;
            message.trid = trid;

            message.process = rm1.process;
            local::send::tm( message);

            message.process = rm2.process;
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
            auto remote_prepare = [&]( auto& involved){

               common::message::transaction::resource::prepare::Request message;

               communication::device::blocking::receive( involved.inbound, message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::resource_error;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_prepare( rm1);
            remote_prepare( rm2);
         };

         // remote rollback
         {
            auto remote_commit = [&]( auto& involved){

               common::message::transaction::resource::rollback::Request message;

               communication::device::blocking::receive( involved.inbound, message);

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.flags == common::flag::xa::Flag::no_flags);

               auto reply = common::message::reverse::type( message);
               reply.resource = message.resource;
               reply.state = common::code::xa::ok;
               reply.trid = message.trid;

               local::send::tm( reply);
            };

            remote_commit( rm1);
            remote_commit( rm2);
         };

         // resource commit reply
         {
            common::message::transaction::resource::commit::Reply message;

            communication::device::blocking::receive( communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.state == common::code::xa::rollback_other) << CASUAL_NAMED_VALUE( message.state);
         }
      }



      TEST( transaction_manager_branch, begin_commit_transaction__1_branched_resource_involved___expect_one_phase_commit_optimization)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // branch involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::id::branch( common::transaction::Context::instance().current().trid);
            message.process = process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);


         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_metrics( state);
         auto& rm1 = proxies.at( 0);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == local::rm_1);
         EXPECT_TRUE( rm1.name == "rm1");
         EXPECT_TRUE( rm1.metrics.resource.count == 1) << CASUAL_NAMED_VALUE( rm1);
      }

   
      TEST( transaction_manager_branch, begin_commit_transaction__rm1_involved__rm1_branched_involved___expect_tpc)
      {
         common::unittest::Trace trace;

         local::Domain domain;


         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         // branch involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::id::branch( common::transaction::Context::instance().current().trid);
            message.process = process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);


         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_metrics( state);
         auto& rm1 = proxies.at( 0);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == local::rm_1);
         EXPECT_TRUE( rm1.name == "rm1");
         EXPECT_TRUE( rm1.metrics.resource.count == 4) << CASUAL_NAMED_VALUE( rm1);  // 2 prepare, 2 commit
      }

      TEST( transaction_manager_branch, begin_commit_transaction__rm1_involved__rm2_branched_involved___expect_tpc)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::Context::instance().current().trid;
            message.process = process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         // branch involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::id::branch( common::transaction::Context::instance().current().trid);
            message.process = process::handle();
            message.involved = { local::rm_2};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);

         state = local::admin::call::state();
         EXPECT_TRUE( state.transactions.empty());

         auto proxies = local::accumulate_metrics( state);
         auto& rm1 = proxies.at( 0);

         ASSERT_TRUE( rm1.instances.size() == 2);
         EXPECT_TRUE( rm1.id == local::rm_1);
         EXPECT_TRUE( rm1.name == "rm1");
         EXPECT_TRUE( rm1.metrics.resource.count == 2) << CASUAL_NAMED_VALUE( rm1);  // 1 prepare, 1 commit
      }

      TEST( transaction_manager_branch, remote_transaction__branched_rm1_involved____expect_original_trid_prepare_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto trid = common::transaction::id::create( process::handle());

         // involved (new branch)
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::id::branch( trid);
            message.process = process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         // remote prepare request
         {
            common::message::transaction::resource::prepare::Request message;
            message.trid = trid;
            message.process = process::handle();
         
            local::send::tm( message);
         }

         // remote prepare reply
         {
            common::message::transaction::resource::prepare::Reply message;

            communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::ok);
         }
      }


      TEST( transaction_manager, one_local_resource__configure)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_NO_THROW({
            common::transaction::context().configure( {}, { "rm1"});
         });
      }


      TEST( transaction_manager, one_local_resource__begin_commit)
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
