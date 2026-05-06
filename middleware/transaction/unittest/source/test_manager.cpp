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
#include "transaction/manager/admin/server.h"
#include "transaction/manager/admin/transform.h"
#include "transaction/context.h"

#include "transaction/unittest/utility.h"
#include "transaction/unittest/rm.h"

#include "common/message/dispatch.h"
#include "common/message/transaction.h"
#include "common/environment.h"
#include "common/transcode.h"
#include "common/functional.h"

#include "common/communication/instance.h"

#include "common/unittest/file.h"
#include "common/unittest/environment.h"


#include "domain/unittest/manager.h"
#include "domain/unittest/configuration.h"

#include "configuration/model/load.h"
#include "configuration/model/transform.h"

#include <fstream>


namespace casual
{
   namespace transaction
   {
      namespace local
      {
         namespace
         {  
            namespace configuration
            {
               constexpr auto system = R"(
system:
   resources:
      -  key: rm-mockup
         server: bin/rm-proxy-casual-mockup
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm
)";

               constexpr auto servers = R"(
domain:
   groups:
      - name: first
      - name: second
        dependencies: [ first]

   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ first]
      - path: bin/casual-transaction-manager
        memberships: [ second]
         
)";

               constexpr auto base = R"(
domain:
   name: transaction-domain

   transaction:
      log: ":memory:"
      resources:
         - key: rm-mockup
           name: rm1
           instances: 2
           openinfo: "${CASUAL_UNITTEST_OPEN_INFO_RM1}"
         - key: rm-mockup
           name: rm2
           instances: 2
           openinfo: "${CASUAL_UNITTEST_OPEN_INFO_RM2}"
)";

               template< typename... C>
               auto load( C&&... contents)
               {
                  auto files = common::unittest::file::temporary::contents( ".yaml", std::forward< C>( contents)...);

                  auto get_path = []( auto& file){ return static_cast< std::filesystem::path>( file);};

                  return casual::configuration::model::load( common::algorithm::transform( files, get_path));
               }

            } // configuration


            template< typename... C>
            auto domain( C&&... configurations) 
            {
               return casual::domain::unittest::manager( configuration::servers, std::forward< C>( configurations)...);
            }

            namespace send
            {
               template< typename M>
               void tm( M&& message)
               {
                  common::communication::device::blocking::send(
                        common::communication::instance::outbound::transaction::manager::device(), message);
               }
            } // send

            namespace call
            {
               template< typename M>
               auto tm( M&& message)
               {
                  return common::communication::ipc::call(
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
                  return action();
               }
               catch( ...)
               {
                  return common::exception::capture().code();
               }
            }

            auto begin() 
            {
               return wrap( [](){ return transaction::context().begin();});
            }

            auto commit() 
            {
               return wrap( [](){ return transaction::context().commit();});
            }

            auto rollback() 
            {
               return wrap( [](){ return transaction::context().rollback();});
            }

            // makes sure the `transaction` is distributed
            void distribute( Transaction& transaction)
            {
               // we fake a 'call' 
               auto correlation = common::strong::correlation::id::generate();
               transaction.associate( correlation);
               transaction.replied( correlation);               
            }


            auto context_clear_scope( std::vector< resource::Link> resources)
            {
               transaction::context().configure( std::move( resources));

               return common::execute::scope( []()
               {
                  transaction::context().clear();
               });
            }

         } // <unnamed>
      } // local


      TEST( transaction_manager, shutdown)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            auto domain = local::domain( local::configuration::system, local::configuration::base);
         });
      }

      TEST( transaction_manager, non_existent_RM_proxy___expect_boot)
      {
         common::unittest::Trace trace;

         constexpr auto resources = R"(
system:
   resources:
      -  key: rm-mockup   
         server: "./non/existent/path"
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm
)";

         EXPECT_NO_THROW({
            auto domain = local::domain( resources, local::configuration::base);
         });
      }


      TEST( transaction_manager, one_RM_xa_open__error___expect_boot)
      {
         common::unittest::Trace trace;

         // we set unittest environment variable to set "error"
         auto scope = common::unittest::environment::scoped::variable( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            common::string::compose( "--open ", XAER_RMFAIL));

         EXPECT_NO_THROW({
            auto domain = local::domain( local::configuration::system, local::configuration::base);
         });
      }


      TEST( transaction_manager, configuration_resource_alias_request)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: transaction-domain

   transaction:
      log: ':memory:'
      resources:
         - key: rm-mockup
           name: rm1
           instances: 2
         - key: rm-mockup
           name: rm2
           instances: 2
           openinfo: openinfo2

)";

         auto domain = local::domain( local::configuration::system, configuration);

         common::message::transaction::configuration::alias::Request request{ common::process::handle()};
         request.resources = { "rm2"};

         auto reply = common::communication::ipc::call( common::communication::instance::outbound::transaction::manager::device(), request);

         ASSERT_TRUE( reply.resources.size() == 1);
         EXPECT_TRUE( reply.resources.at( 0).name == "rm2");
         EXPECT_TRUE( reply.resources.at( 0).openinfo == "openinfo2");
      }

      TEST( transaction_manager, configuration_get)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: configuration_get

   transaction:
      log: ':memory:'
      resources:
         - key: rm-mockup
           name: a
           instances: 1
           openinfo: "openinfo a"
           note: a
         - key: rm-mockup
           name: b
           instances: 2
           openinfo: "openinfo b"
           note: b
)";
         
         auto domain = local::domain( local::configuration::system, configuration);

         auto origin = local::configuration::load( local::configuration::servers, configuration).transaction;

         auto model = casual::configuration::model::transform( casual::domain::unittest::configuration::get()).transaction;

         EXPECT_TRUE( origin == model) << CASUAL_NAMED_VALUE( origin) << '\n' << CASUAL_NAMED_VALUE( model);

      }

      TEST( transaction_manager, configuration_post)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, R"(
domain:
   name: post

   transaction:
      log: ':memory:'
      resources:
         - key: rm-mockup
           name: a
           instances: 1
           openinfo: "openinfo a"
           note: a
         - key: rm-mockup
           name: b
           instances: 2
           openinfo: "openinfo b"
           note: b
)");
         
         auto wanted = local::configuration::load( local::configuration::system, local::configuration::servers, R"(
domain:
   name: post

   transaction:
      log: ':memory:'
      resources:
         - name: a
           key: rm-mockup
           note: modified
           instances: 3
           openinfo: "openinfo modified"
         - key: rm-mockup
           name: x
           instances: 3
           openinfo: "openinfo x"
           note: a
         - key: rm-mockup
           name: y
           instances: 1
           openinfo: "openinfo y"
           note: b
)");

         // make sure the wanted differs (otherwise we're not testing anyting...)
         ASSERT_TRUE( wanted.transaction != casual::configuration::model::transform( casual::domain::unittest::configuration::get()).transaction);

         // post the wanted model (in transformed user representation)
         auto updated = casual::configuration::model::transform( 
            casual::domain::unittest::configuration::post( casual::configuration::model::transform( wanted)));

         EXPECT_TRUE( wanted.transaction == updated.transaction) << CASUAL_NAMED_VALUE( wanted.transaction) << '\n' << CASUAL_NAMED_VALUE( updated.transaction);

      }
      


      TEST( transaction_manager, begin_transaction)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         auto state = unittest::state();

         EXPECT_TRUE( state.transactions.empty()) << CASUAL_NAMED_VALUE( state.transactions);

         EXPECT_TRUE( local::commit() == common::code::tx::ok);
      }


      TEST( transaction_manager, commit_transaction__expect_ok__no_resource_roundtrips)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         common::log::debug( "domain: ", domain);


         EXPECT_TRUE( local::begin() == common::code::tx::ok);
         EXPECT_TRUE( local::commit() == common::code::tx::ok);

         auto state = unittest::state();
         EXPECT_TRUE( state.transactions.empty());

         for( auto& resource : state.resources)
            for( auto& instance : resource.instances)
               EXPECT_TRUE( instance.metrics.resource.count == 0);
      }


      TEST( transaction_manager, begin_commit_transaction__1_resources_involved__expect_one_phase_commit_optimization)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         local::distribute( transaction::context().current());
 

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);

         auto state = unittest::state();
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

         auto domain = local::domain( local::configuration::system, local::configuration::base);


         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         local::distribute( transaction::context().current());



         // first time involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         // second time involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            ASSERT_TRUE( reply.involved.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.involved.at( 0) == local::rm_1);
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);

         auto state = unittest::state();
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

      // The removed testcase transaction_manager, begin_commit_transaction__1_resources_involved__xa_XA_RBDEADLOCK___expect__TX_HAZARD
      // is not correct. XA_RBDEADLOCK is a return code from the RM, and should be mapped to TX_ROLLBACK, not TX_HAZARD,
      // since the resource has done rollback.

      TEST( transaction_manager, begin_commit_transaction__1_resources_involved__tx_commit_XA_RBDEADLOCK___expect__TX_ROLLBACK)
      {
         common::unittest::Trace trace;


         // we set unittest environment variable to set "error"
         auto scope = common::unittest::environment::scoped::variable( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            common::string::compose( "--commit ", XA_RBDEADLOCK));

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_EQ( local::begin(), common::code::tx::ok);

         // Make sure we make the transaction distributed
         local::distribute( transaction::context().current());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_EQ( local::commit(), common::code::tx::rollback);
      }


      TEST( transaction_manager, no_transaction__1_resources_involved__XAER_NOTA___expect__TX_OK)
      {
         common::unittest::Trace trace;

         // we set unittest environment variable to set "error"
         auto scope = common::unittest::environment::scoped::variable( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            common::string::compose( "--commit ", XAER_NOTA));

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         local::distribute( transaction::context().current());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }
         EXPECT_EQ( local::commit(), common::code::tx::ok);
      }

      TEST( transaction_manager, begin_commit_transaction__1_resources_involved__environment_open_info__XAER_NOTA___expect__TX_OK)
      {
         common::unittest::Trace trace;

         // we set unittest environment variable to set "error"
         auto scope = common::unittest::environment::scoped::variable( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            common::string::compose( "--commit ", XAER_NOTA));

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         local::distribute( transaction::context().current());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);
      }

      TEST( transaction_manager, begin_rollback_transaction__1_resources_involved__expect_XA_OK)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         local::distribute( transaction::context().current());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }     

         EXPECT_TRUE( local::rollback() == common::code::tx::ok);

         auto state = unittest::state();
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

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         local::distribute( transaction::context().current());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1, local::rm_2};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_TRUE( local::rollback() == common::code::tx::ok);

         auto state = unittest::state();
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

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         local::distribute( transaction::context().current());

         // first rm involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         // second rm involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_2};
            
            auto reply = local::call::tm( message);

            //! should give the first rm as already involved
            ASSERT_TRUE( reply.involved.size() == 1);
            EXPECT_TRUE( reply.involved.at( 0) == local::rm_1);
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);

         auto state = unittest::state();
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

      namespace local
      {
         namespace
         {
            namespace involved
            {
               struct Resource 
               {
                  common::communication::ipc::inbound::Device inbound;
                  common::process::Handle process() const { return { common::process::id(), inbound.connector().handle().ipc()};}
               };

            } // involved
         } // <unnamed>
      } // local


      TEST( transaction_manager, begin_transaction__2_resource_involved__owner_dies__expect_rollback)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1, local::rm_2};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         // caller dies
         {
            common::message::event::process::Exit event;
            event.state.pid = common::process::handle().pid;
            event.state.reason = decltype( event.state.reason)::core;

            local::send::tm( event);
         }
         
         // TODO unittest replace with fetch and predicate
         // should be more than enough for TM to complete the rollback.
         common::process::sleep( std::chrono::milliseconds{ 10});

         auto state = unittest::state();

         // transaction should be rolled back and removed
         EXPECT_TRUE( state.transactions.empty()) << CASUAL_NAMED_VALUE( state.transactions);

         EXPECT_TRUE( local::rollback() == common::code::tx::ok);
      }

      TEST( transaction_manager, two_external_involved__prepare_requests___ipc_destroyed_event___expect__TX_FAIL)
      {
         common::unittest::Trace trace;

         local::involved::Resource r1;
         local::involved::Resource r2;

         auto involve_external = []( auto& resource, auto& trid)
         {
            common::message::transaction::resource::external::Involved message{ resource.process()};
            message.trid = trid;
            local::send::tm( message);
         };

         auto reply_to_tm = []< typename M>( auto& resource, M)
         {
            auto request = common::communication::device::receive< M>( resource.inbound);
            auto reply = common::message::reverse::type( request, resource.process());
            reply.resource = request.resource;
            reply.trid = request.trid;
            local::send::tm( reply);
         };

         auto domain = local::domain();

         auto trid = common::transaction::id::create();
         
         involve_external( r1, trid);
         involve_external( r2, trid);

         // send to TM to start the commit dance
         { 
            common::message::transaction::commit::Request message{ common::process::handle()};
            message.trid = trid;
            local::send::tm( message);
         }

         reply_to_tm( r1, common::message::transaction::resource::prepare::Request{});

         // r2 has failed
         {
            local::send::tm( common::message::event::ipc::Destroyed{ r2.process()});
         }

         // we expect rollback to r1
         reply_to_tm( r1, common::message::transaction::resource::rollback::Request{});

         // we expect commit failed
         {
            auto reply = common::communication::ipc::receive< common::message::transaction::commit::Reply>();
            EXPECT_TRUE( reply.state == decltype( reply.state)::fail) << CASUAL_NAMED_VALUE( reply);
         }
      }

      TEST( transaction_manager, remote_resource_commit_one_phase__xid_unknown___expect_read_only)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto trid = common::transaction::id::create();


         // remote commit
         {
            common::message::transaction::resource::commit::Request message;
            message.trid = trid;
            message.process = common::process::handle();
            message.flags = common::flag::xa::Flag::one_phase;

            local::send::tm( message);
         }

         // commit reply
         {
            common::message::transaction::resource::commit::Reply message;

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid) << "trid: " << trid << "\nmessage.trid: " << message.trid;
            EXPECT_TRUE( message.state == common::code::xa::read_only);
         }
      }

      TEST( transaction_manager, remote_resource_prepare__xid_unknown___expect_read_only)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto trid = common::transaction::id::create();


         // remote commit
         {
            common::message::transaction::resource::prepare::Request message;
            message.trid = trid;
            message.process = common::process::handle();
         
            local::send::tm( message);
         }

         // commit reply
         {
            common::message::transaction::resource::prepare::Reply message;

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::read_only);
         }
      }

      TEST( transaction_manager, remote_resource_rollback__xid_unknown___expect_xa_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto trid = common::transaction::id::create();


         // remote rollback
         {
            common::message::transaction::resource::rollback::Request message;
            message.trid = trid;
            message.process = common::process::handle();
         
            local::send::tm( message);
         }

         // commit reply
         {
            common::message::transaction::resource::rollback::Reply message;

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            // we expect to get read_only, altough the rm classifies this as an error...
            EXPECT_TRUE( message.state == common::code::xa::read_only) << CASUAL_NAMED_VALUE( message);
         }
      }


      TEST( transaction_manager, remote_owner__local_resource_involved__remote_rollback____expect_rollback)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto trid = common::transaction::id::create();


         // local involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty()) << CASUAL_NAMED_VALUE( reply.involved);
         }

         // remote rollback
         {
            common::message::transaction::resource::rollback::Request message;
            message.trid = trid;
            message.process = common::process::handle();

            local::send::tm( message);
         }

         // rollback reply
         {
            common::message::transaction::resource::rollback::Reply message;

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::ok) << CASUAL_NAMED_VALUE( message.state);
         }
      }

      TEST( transaction_manager, remote_owner__same_remote_resource_involved__remote_rollback____expect_xa_read_only)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto trid = common::transaction::id::create();

         constexpr auto resource = common::strong::resource::id{ -200}; 


         // remote involved
         {
            common::message::transaction::resource::external::Involved message{ common::process::handle()};
            message.trid = trid;

            local::send::tm( message);
         }

         // remote rollback request
         {
            common::message::transaction::resource::rollback::Request message{ common::process::handle()};
            message.trid = trid;
            message.resource = resource;

            local::send::tm( message);
         }

         // we will get a rollback request from TM since we pretend to be an involved remote resource
         {
            common::message::transaction::resource::rollback::Request request;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), request);

            EXPECT_TRUE( request.resource == common::strong::resource::id{ -1});

            auto reply = common::message::reverse::type( request);
            reply.trid = request.trid;
            reply.state = decltype( reply.state)::read_only;
            common::communication::device::blocking::send( request.process.ipc, reply);
         }

         // remote rollback reply from TM
         {
            common::message::transaction::resource::rollback::Reply message;

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.resource == resource) << CASUAL_NAMED_VALUE( message.resource);
            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::read_only) << CASUAL_NAMED_VALUE( message.state);
         }
      }

      TEST( transaction_manager, remote_owner__one_resource_involved__prepare_request__expects_no_commit_to_resource_before_prepare_reply)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto trid = common::transaction::id::create();

         constexpr auto resource = common::strong::resource::id{ -200}; 


         // remote involved
         {
            common::message::transaction::resource::external::Involved message{ common::process::handle()};
            message.trid = trid;

            local::send::tm( message);
         }

         // remote prepare request
         {
            common::message::transaction::resource::prepare::Request message{ common::process::handle()};
            message.trid = trid;
            message.resource = resource;

            local::send::tm( message);
         }

         // we will get a prepare request from TM since we pretend to be an involved remote resource
         {
            auto request = common::communication::ipc::receive< common::message::transaction::resource::prepare::Request>();
            EXPECT_TRUE( request.trid == trid);

            // the first involved "external" resource gets "E-1" (-1)
            EXPECT_TRUE( request.resource == common::strong::resource::id{ -1});

            auto reply = common::message::reverse::type( request);
            reply.trid = request.trid;
            reply.resource = request.resource;
            reply.state = decltype( reply.state)::ok;
            common::communication::device::blocking::send( request.process.ipc, reply);
         }

         // we get the prepare reply from TM. We know that TM could not do something crazy as start committing the resource
         // since we would not get the prepare reply until we replied to commit/rollback as the resource 
         {
            auto reply = common::communication::ipc::receive< common::message::transaction::resource::prepare::Reply>();
            EXPECT_TRUE( reply.trid == trid);
            EXPECT_TRUE( reply.state == decltype( reply.state)::ok);
         }

      }

      TEST( transaction_manager, begin_transaction__1_remote_resource_involved___expect_one_phase_optimization)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto trid = common::transaction::id::create();

         // external involved
         {
            common::message::transaction::resource::external::Involved message;
            message.trid = trid;
            message.process = common::process::handle();

            local::send::tm( message);
         }

         // commit
         {
            common::message::transaction::commit::Request message;
            message.trid = trid;
            message.process = common::process::handle();

            local::send::tm( message);
         }

         // remote commit (one phase optimization)
         {
            common::message::transaction::resource::commit::Request message;

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

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

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

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
                  return common::strong::process::id{ ++global};
               }

               struct Process 
               {
                  common::communication::ipc::inbound::Device inbound;
                  common::process::Handle process{ next(), inbound.connector().handle().ipc()};
               };


            } // involved
         } // <unnamed>
      } // local

      TEST( transaction_manager, begin_transaction__2_remote_resource_involved___expect_remote_prepare_commit)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);


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
            message.process = common::process::handle();

            local::send::tm( message);
         }

         // remote prepare
         {
            auto remote_prepare = [&]( auto& involved){

               common::message::transaction::resource::prepare::Request message;

               common::communication::device::blocking::receive( involved.inbound, message);

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

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.stage == decltype( message.stage)::prepare);
            EXPECT_TRUE( message.state == common::code::tx::ok);
         }


         // remote commit
         {
            auto remote_commit = [&]( auto& involved){

               common::message::transaction::resource::commit::Request message;

               common::communication::device::blocking::receive( involved.inbound, message);

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

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.stage == decltype( message.stage)::commit);
            EXPECT_TRUE( message.state == common::code::tx::ok);
         }
      }

      TEST( transaction_manager, begin_transaction__2_remote_resource_involved_read_only___expect_remote_prepare__read_only_optimization)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

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
            message.process = common::process::handle();

            local::send::tm( message);
         }


         // remote prepare
         {
            auto remote_prepare = [&]( auto& involved){

               common::message::transaction::resource::prepare::Request message;

               common::communication::device::blocking::receive( involved.inbound, message);

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

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.state == common::code::tx::ok) << CASUAL_NAMED_VALUE( message.state);
         }
      }


      TEST( transaction_manager, transaction_2_remote_resource_involved__one_phase_commit_optimization___expect_prepare_phase_commit_XA_OK)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

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
            message.process = common::process::handle();
            message.flags = common::flag::xa::Flag::one_phase;

            local::send::tm( message);
         }


         // remote prepare
         {
            auto remote_prepare = [&]( auto& involved)
            {
               common::message::transaction::resource::prepare::Request message;

               common::communication::device::blocking::receive( involved.inbound, message);

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

               common::communication::device::blocking::receive( involved.inbound, message);

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

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.state == common::code::xa::ok) << CASUAL_NAMED_VALUE( message.state);
         }
      }

      TEST( transaction_manager, transaction_2_remote_resource_involved__one_phase_commit_optimization__RM_fail__expect_rollback__commit_XAER_RMERR)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

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
            message.process = common::process::handle();
            message.flags = common::flag::xa::Flag::one_phase;

            local::send::tm( message);
         }


         // remote prepare
         {
            auto remote_prepare = [&]( auto& involved){

               common::message::transaction::resource::prepare::Request message;

               common::communication::device::blocking::receive( involved.inbound, message);

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

               common::communication::device::blocking::receive( involved.inbound, message);

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

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.state == common::code::xa::resource_error) << CASUAL_NAMED_VALUE( message.state);
         }
      }



      TEST( transaction_manager_branch, begin_commit_transaction__1_branched_resource_involved___expect_one_phase_commit_optimization)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         local::distribute( transaction::context().current());

         // branch involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::id::branch( transaction::context().current().trid);
            message.process = common::process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);


         auto state = unittest::state();
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

         auto domain = local::domain( local::configuration::system, local::configuration::base);


         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         local::distribute( transaction::context().current());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         // branch involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::id::branch( transaction::context().current().trid);
            message.process = common::process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);


         auto state = unittest::state();
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

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         local::distribute( transaction::context().current());

         // involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = transaction::context().current().trid;
            message.process = common::process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         // branch involved
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::id::branch( transaction::context().current().trid);
            message.process = common::process::handle();
            message.involved = { local::rm_2};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         EXPECT_TRUE( local::commit() == common::code::tx::ok);

         auto state = unittest::state();
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

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto trid = common::transaction::id::create( common::process::id());

         // involved (new branch)
         {
            common::message::transaction::resource::involved::Request message;
            message.trid = common::transaction::id::branch( trid);
            message.process = common::process::handle();
            message.involved = { local::rm_1};

            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty());
         }

         // remote prepare request
         {
            common::message::transaction::resource::prepare::Request message;
            message.trid = trid;
            message.process = common::process::handle();
         
            local::send::tm( message);
         }

         // remote prepare reply
         {
            common::message::transaction::resource::prepare::Reply message;

            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::ok);
         }
      }

      TEST( transaction_manager, 1_local_RM_xa_start__error___begin____expect_TX_ERROR__no_started_transaction)
      {
         common::unittest::Trace trace;

         // we set unittest environment variable to set "error"
         auto scope = common::unittest::environment::scoped::variable( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            common::string::compose( "--start ", XAER_RMFAIL));

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         // configure the local rm - will get XAER_RMFAIL on xa_start
         transaction::context().configure( { { "rm-mockup", "rm1", &casual_mockup_xa_switch_static}});

         EXPECT_TRUE( local::begin() == common::code::tx::error);

         EXPECT_TRUE( ! transaction::context().current()) << CASUAL_NAMED_VALUE( transaction::context().current());

         // unittest only...
         transaction::context().clear();
      }
      

      TEST( transaction_manager, local_transaction__two_resources__expect_distributed_transaction)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto count_invocation = []( auto id, auto type) 
         {
            return common::algorithm::count( unittest::rm::state::get( id).invocations, type);
         };

         constexpr auto rm1 = common::strong::resource::id{ 1};
         constexpr auto rm2 = common::strong::resource::id{ 2};

         // configure the local rm:s
         transaction::context().configure( { 
            { "rm-mockup", "rm1", &casual_mockup_xa_switch_static},
            { "rm-mockup", "rm2", &casual_mockup_xa_switch_static}});

         EXPECT_TRUE( count_invocation( rm1, unittest::rm::state::Invoke::xa_open_entry) == 1);
         EXPECT_TRUE( count_invocation( rm2, unittest::rm::state::Invoke::xa_open_entry) == 1);
         EXPECT_TRUE( count_invocation( rm1, unittest::rm::state::Invoke::xa_start_entry) == 0);
         EXPECT_TRUE( count_invocation( rm2, unittest::rm::state::Invoke::xa_start_entry) == 0);

         // begin transaction
         {
            EXPECT_TRUE( local::begin() == common::code::tx::ok);
            EXPECT_TRUE( count_invocation( rm1, unittest::rm::state::Invoke::xa_start_entry) == 1);
            EXPECT_TRUE( count_invocation( rm2, unittest::rm::state::Invoke::xa_start_entry) == 1);
         }

         // commit transaction
         {
            EXPECT_TRUE( local::commit() == common::code::tx::ok);
            EXPECT_TRUE( count_invocation( rm1, unittest::rm::state::Invoke::xa_end_entry) == 1);
            EXPECT_TRUE( count_invocation( rm2, unittest::rm::state::Invoke::xa_end_entry) == 1);

            auto resource_proxy_invoked = []( auto& state, auto id) -> decltype( state.resources.at( 0))
            {
               if( auto found = common::algorithm::find( state.resources, id))
                  return *found;

               common::code::raise::error( common::code::casual::invalid_argument, "failed to find ", id);
            };

            auto state = unittest::state();
            auto& state_rm1 = resource_proxy_invoked( state, rm1);
            auto& state_rm2 = resource_proxy_invoked( state, rm2);
            EXPECT_TRUE( state_rm1.instances.at( 0).metrics.resource.count == 2) << CASUAL_NAMED_VALUE( state_rm1);
            EXPECT_TRUE( state_rm2.instances.at( 0).metrics.resource.count == 2) << CASUAL_NAMED_VALUE( state_rm2);
         }

         // unittest only...
         transaction::context().clear();
      }

      TEST( transaction_manager, two_resources__send_prepare_request__one_external_resource__send_one_phase_commit__expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         local::involved::Process rm1;
         local::involved::Process rm2;

         auto trid = common::transaction::id::create( common::process::id());
         auto branch = common::transaction::id::branch( trid);

         // involve resource 1 and 2, resource 2 is an outbound
         {
            common::message::transaction::resource::external::Involved message;
            
            message.process = rm1.process;
            message.trid = trid;
            local::send::tm( message);

            message.process = rm2.process;
            message.trid = branch;
            local::send::tm( message);
         }

         // act as "user" and send commit::Request to transaction manager
         {
            common::message::transaction::commit::Request message;
            message.trid = trid;
            message.process = common::process::handle();

            local::send::tm( message);
         }

         // resource 1 receives a resource::prepare::Request from transaction manager
         // and sends a resource::prepare::Reply xa::ok back to transaction manager
         {
            common::message::transaction::resource::prepare::Request message;
            common::communication::device::blocking::receive( rm1.inbound, message);

            auto reply = common::message::reverse::type( message);

            reply.resource = message.resource;
            reply.state = common::code::xa::ok;
            reply.trid = message.trid;

            local::send::tm( reply);
         }

         {
            // resource 2 (outbound) receives a resource::prepare::Request from transaction manager
            common::message::transaction::resource::prepare::Request message;
            common::communication::device::blocking::receive( rm2.inbound, message);

            // act as another domains transaction manager that got a resource::prepare::Request
            // and send a resource::commit::Request (one-phase-optimization) to the first transaction manager
            {
               common::message::transaction::resource::commit::Request message;
               message.trid = branch;
               message.process = common::process::handle();
               message.flags = common::flag::xa::Flag::one_phase;

               local::send::tm( message);
            }

            // receive resource::commit::Reply from transaction manager
            {
               auto message = common::communication::ipc::receive< common::message::transaction::resource::commit::Reply>();

               EXPECT_TRUE( message.trid == branch);
               EXPECT_TRUE( message.state == decltype( message.state)::read_only);
            }

            // send resource::prepare::Reply xa::ok from resource 2 (outbound) to transaction manager
            auto reply = common::message::reverse::type( message);

            reply.resource = message.resource;
            reply.state = common::code::xa::ok;
            reply.trid = message.trid;

            local::send::tm( reply);
         }

         // commit::Reply from transaction manager
         {
            auto message = common::communication::ipc::receive< common::message::transaction::commit::Reply>();

            EXPECT_TRUE( message.trid == trid) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.state == decltype( message.state)::ok) << CASUAL_NAMED_VALUE( message.state);
         }

      }

      TEST( transaction_manager, two_resources__send_prepare_request__one_external_resource__send_prepare_request__expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         local::involved::Process rm1;
         local::involved::Process rm2;

         auto trid = common::transaction::id::create( common::process::id());
         auto branch = common::transaction::id::branch( trid);

         // involve resource 1 and 2, resource 2 is an outbound
         {
            common::message::transaction::resource::external::Involved message;
            
            message.process = rm1.process;
            message.trid = trid;
            local::send::tm( message);

            message.process = rm2.process;
            message.trid = branch;
            local::send::tm( message);
         }

         // act as "user" and send commit::Request to transaction manager
         {
            common::message::transaction::commit::Request message;
            message.trid = trid;
            message.process = common::process::handle();

            local::send::tm( message);
         }

         // resource 1 receives a resource::prepare::Request from transaction manager
         // and sends a resource::prepare::Reply xa::ok back to transaction manager
         {
            common::message::transaction::resource::prepare::Request message;
            common::communication::device::blocking::receive( rm1.inbound, message);

            auto reply = common::message::reverse::type( message);

            reply.resource = message.resource;
            reply.state = common::code::xa::ok;
            reply.trid = message.trid;

            local::send::tm( reply);
         }

         {
            // resource 2 (outbound) receives a resource::prepare::Request from transaction manager
            common::message::transaction::resource::prepare::Request message;
            common::communication::device::blocking::receive( rm2.inbound, message);

            // act as another domains transaction manager that got a resource::prepare::Request
            // and send a resource::prepare::Request to the first transaction manager
            {
               common::message::transaction::resource::prepare::Request message;
               message.trid = branch;
               message.process = common::process::handle();

               local::send::tm( message);
            }

            // receive resource::prepare::Reply from transaction manager
            {
               auto message = common::communication::ipc::receive< common::message::transaction::resource::prepare::Reply>();

               EXPECT_TRUE( message.trid == branch);
               EXPECT_TRUE( message.state == decltype( message.state)::read_only);
            }

            // send resource::prepare::Reply xa::ok from resource 2 (outbound) to transaction manager
            auto reply = common::message::reverse::type( message);

            reply.resource = message.resource;
            reply.state = common::code::xa::ok;
            reply.trid = message.trid;

            local::send::tm( reply);
         }

         // commit::Reply from transaction manager
         {
            auto message = common::communication::ipc::receive< common::message::transaction::commit::Reply>();

            EXPECT_TRUE( message.trid == trid) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.state == decltype( message.state)::ok) << CASUAL_NAMED_VALUE( message.state);
         }

      }

      TEST( transaction_manager, one_resources__send_commit_request__one_external_resource__send_prepare_request__expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         local::involved::Process rm1;

         auto trid = common::transaction::id::create( common::process::id());

         // involve resource 1, resource 1 is an outbound
         {
            common::message::transaction::resource::external::Involved message;
            
            message.process = rm1.process;
            message.trid = trid;
            local::send::tm( message);
         }

         // act as "user" and send commit::Request to transaction manager
         {
            common::message::transaction::commit::Request message;
            message.trid = trid;
            message.process = common::process::handle();

            local::send::tm( message);
         }

         {
            // resource 1 (outbound) receives a resource::commit::Request from transaction manager
            common::message::transaction::resource::commit::Request message;
            common::communication::device::blocking::receive( rm1.inbound, message);

            // act as another domains transaction manager that got a resource::prepare::Request
            // and send a resource::prepare::Request to the first transaction manager
            {
               common::message::transaction::resource::prepare::Request message;
               message.trid = trid;
               message.process = common::process::handle();

               local::send::tm( message);
            }

            // receive resource::prepare::Reply from transaction manager
            {
               auto message = common::communication::ipc::receive< common::message::transaction::resource::prepare::Reply>();

               EXPECT_TRUE( message.trid == trid);
               EXPECT_TRUE( message.state == decltype( message.state)::read_only);
            }

            // send resource::commit::Reply xa::ok from resource 1 (outbound) to transaction manager
            auto reply = common::message::reverse::type( message);

            reply.resource = message.resource;
            reply.state = common::code::xa::ok;
            reply.trid = message.trid;

            local::send::tm( reply);
         }

         // commit::Reply from transaction manager
         {
            auto message = common::communication::ipc::receive< common::message::transaction::commit::Reply>();

            EXPECT_TRUE( message.trid == trid) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.state == decltype( message.state)::ok) << CASUAL_NAMED_VALUE( message.state);
         }
      }

      namespace local
      {
         namespace
         {
            void involve_resources( auto& resources, auto& trid)
            {
               for( auto& resource : resources)
               {
                  common::message::transaction::resource::external::Involved message;
                  message.process = resource.process;
                  message.trid = trid;
                  local::send::tm( message);
               }
            };

            void handle_rollback_request( auto& resources)
            {
               for( auto& resource : resources)
               {
                  auto request = common::unittest::fetch::message::until< common::message::transaction::resource::rollback::Request>( resource.inbound);
                  auto reply = common::message::reverse::type( request);
                  reply.trid = request.trid;
                  reply.resource = request.resource;
                  reply.state = common::code::xa::ok;
                  local::send::tm( reply);
               }
            };

            
         } // <unnamed>
      } // local

      TEST( transaction_manager, two_resources_involved__send_potential_stale__expect_rollback)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto resources = std::array< local::involved::Process, 2>();

         auto trid = common::transaction::id::create( common::process::id());

         local::involve_resources( resources, trid);


         // send the potentially stale message, this will trigger a rollback.
         {
            common::message::transaction::potential::Stale message{ common::process::handle()};
            message.gtrid = common::transaction::global::ID{ trid.global()};
            local::send::tm( message);
         }

         local::handle_rollback_request( resources);

      }

      TEST( transaction_manager, ongoing_rollback__two_resources_involved___expect_stale_rollback)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto resources = std::array< local::involved::Process, 2>();

         auto trid = common::transaction::id::create( common::process::id());

         local::involve_resources( resources, trid);

         {
            common::message::transaction::rollback::Request message{ common::process::handle()};
            message.trid = trid;
            local::send::tm( message);
         }

         // TM will send rollback requests to our resources. We'll involve them agin -> they will be treated as stale.
         local::involve_resources( resources, trid);

         // verify that the resources are stale
         {
            auto state = unittest::state();
            EXPECT_TRUE( state.stale.size() == 1);
            EXPECT_TRUE( state.stale.at( 0).branches.at( 0).resources.size() == 2);
         }

         // handle the first ongoing rollback request
         local::handle_rollback_request( resources);

         // handle the second stale rollback request
         local::handle_rollback_request( resources);
      }

      TEST( transaction_manager, local_resource__rollback__xa_end__XA_RBTIMEOUT__expect_promoted_to_distributed_rollback)
      {
         common::unittest::Trace trace;

         static_assert( XA_RBTIMEOUT == 106);

         auto domain = local::domain( R"(
system:
   resources:
      -  key: rm-mockup
         server: bin/rm-proxy-casual-mockup
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm
domain:
   name: A

   transaction:
      log: ":memory:"
      resources:
         - key: rm-mockup
           name: rm1
           instances: 1
           openinfo: "--end 106" # XA_RBTIMEOUT
)");

         auto scope = local::context_clear_scope( { { "rm-mockup", "rm1", &casual_mockup_xa_switch_static}});

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         EXPECT_TRUE( local::rollback() == common::code::tx::ok);

         // we check that the local resource only got xa_open, xa_start, and xa_end. No xa_rollback should
         // be local, but promoted to distributed.
         {  
            // we only got one resource, hence id 1
            auto& invocations = transaction::unittest::rm::state::get( common::strong::resource::id{ 1}).invocations;
            EXPECT_TRUE( invocations.size() == 3) << CASUAL_NAMED_VALUE( invocations);
            EXPECT_TRUE( invocations.at( 0) == unittest::rm::state::Invoke::xa_open_entry) << CASUAL_NAMED_VALUE( invocations.at( 0));
            EXPECT_TRUE( invocations.at( 1) == unittest::rm::state::Invoke::xa_start_entry) << CASUAL_NAMED_VALUE( invocations.at( 1));
            EXPECT_TRUE( invocations.at( 2) == unittest::rm::state::Invoke::xa_end_entry) << CASUAL_NAMED_VALUE( invocations.at( 2));

         }
          
         // check that the resource proxy got xa_rollback
         {
            auto state = unittest::state();
            EXPECT_TRUE( state.resources.at( 0).instances.at( 0).metrics.resource.count == 1); // xa_rollback
         }
      }
   
   } // transaction

} // casual
