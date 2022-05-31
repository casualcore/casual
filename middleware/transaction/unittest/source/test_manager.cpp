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

#include "transaction/unittest/utility.h"

#include "common/message/dispatch.h"
#include "common/message/transaction.h"
#include "common/environment.h"
#include "common/transcode.h"
#include "common/functional.h"
#include "common/environment/scoped.h"

#include "common/communication/instance.h"

#include "common/unittest/file.h"
#include "common/unittest/rm.h"


#include "domain/unittest/manager.h"
#include "domain/unittest/configuration.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/log.h"

#include "configuration/model/load.h"
#include "configuration/model/transform.h"

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
                  return exception::capture().code();
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
         auto scope = common::environment::variable::scoped::set( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
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

         common::log::line( verbose::log, "domain: ", domain);


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
         auto state = unittest::state();
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

         state = unittest::state();
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
         auto state = unittest::state();
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

         state = unittest::state();
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
         auto scope = common::environment::variable::scoped::set( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            common::string::compose( "--commit ", XA_RBDEADLOCK));

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = unittest::state();
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
         auto scope = common::environment::variable::scoped::set( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            common::string::compose( "--commit ", XAER_NOTA));

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = unittest::state();
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
         EXPECT_EQ( local::commit(), common::code::tx::ok);
      }

      TEST( transaction_manager, begin_commit_transaction__1_resources_involved__environment_open_info__XAER_NOTA___expect__TX_OK)
      {
         common::unittest::Trace trace;

         // we set unittest environment variable to set "error"
         auto scope = common::environment::variable::scoped::set( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            common::string::compose( "--commit ", XAER_NOTA));

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = unittest::state();
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

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = unittest::state();
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

         state = unittest::state();
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
         auto state = unittest::state();
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

         state = unittest::state();
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
         auto state = unittest::state();
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

         state = unittest::state();
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

         auto domain = local::domain( local::configuration::system, local::configuration::base);

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
         
         // TODO unittest replace with fetch and predicate
         // should be more than enough for TM to complete the rollback.
         process::sleep( std::chrono::milliseconds{ 10});

         auto state = unittest::state();

         // transaction should be rolled back and removed
         EXPECT_TRUE( state.transactions.empty()) << CASUAL_NAMED_VALUE( state.transactions);

         EXPECT_TRUE( local::rollback() == common::code::tx::ok);
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

         auto domain = local::domain( local::configuration::system, local::configuration::base);

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

         auto domain = local::domain( local::configuration::system, local::configuration::base);

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
            message.process = process::handle();
            message.involved = { local::rm_1};
            
            auto reply = local::call::tm( message);
            EXPECT_TRUE( reply.involved.empty()) << CASUAL_NAMED_VALUE( reply.involved);
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
            EXPECT_TRUE( message.state == common::code::xa::ok) << CASUAL_NAMED_VALUE( message.state);
         }
      }

      TEST( transaction_manager, remote_owner__same_remote_resource_involved__remote_rollback____expect_xa_read_only)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto trid = common::transaction::id::create();

         constexpr auto resource = strong::resource::id{ -200}; 


         // remote involved
         {
            common::message::transaction::resource::external::Involved message{ process::handle()};
            message.trid = trid;

            local::send::tm( message);
         }

         // remote rollback request
         {
            common::message::transaction::resource::rollback::Request message{ process::handle()};
            message.trid = trid;
            message.resource = resource;

            local::send::tm( message);
         }

         // we will get a rollback request from TM since we pretend to be an involved remote resource
         {
            common::message::transaction::resource::rollback::Request request;
            communication::device::blocking::receive( common::communication::ipc::inbound::device(), request);

            EXPECT_TRUE( request.resource == common::strong::resource::id{ -1});

            auto reply = common::message::reverse::type( request);
            reply.trid = request.trid;
            reply.state = decltype( reply.state)::read_only;
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // remote rollback reply from TM
         {
            common::message::transaction::resource::rollback::Reply message;

            communication::device::blocking::receive( common::communication::ipc::inbound::device(), message);

            EXPECT_TRUE( message.resource == resource) << CASUAL_NAMED_VALUE( message.resource);
            EXPECT_TRUE( message.trid == trid);
            EXPECT_TRUE( message.state == common::code::xa::read_only) << CASUAL_NAMED_VALUE( message.state);
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
            EXPECT_TRUE( message.stage == decltype( message.stage)::prepare);
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
            EXPECT_TRUE( message.state == common::code::xa::resource_error) << CASUAL_NAMED_VALUE( message.state);
         }
      }



      TEST( transaction_manager_branch, begin_commit_transaction__1_branched_resource_involved___expect_one_phase_commit_optimization)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         EXPECT_TRUE( local::begin() == common::code::tx::ok);

         // Make sure we make the transaction distributed
         auto state = unittest::state();
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


         state = unittest::state();
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
         auto state = unittest::state();
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


         state = unittest::state();
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
         auto state = unittest::state();
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

         state = unittest::state();
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

      TEST( transaction_manager, 1_local_RM_xa_start__error___begin____expect_TX_ERROR__no_started_transaction)
      {
         common::unittest::Trace trace;

         // we set unittest environment variable to set "error"
         auto scope = common::environment::variable::scoped::set( "CASUAL_UNITTEST_OPEN_INFO_RM1", 
            common::string::compose( "--start ", XAER_RMFAIL));

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         // configure the local rm - will get XAER_RMFAIL on xa_start
         common::transaction::context().configure( { { "rm-mockup", "rm1", &casual_mockup_xa_switch_static}});

         EXPECT_TRUE( local::begin() == common::code::tx::error);

         EXPECT_TRUE( ! common::transaction::context().current()) << CASUAL_NAMED_VALUE( common::transaction::context().current());

         // unittest only...
         common::transaction::context().clear();
      }
      

      TEST( transaction_manager, local_transaction__two_resources__expect_distributed_transaction)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         auto count_invocation = []( auto id, auto type) 
         {
            return algorithm::count( common::unittest::rm::state( id).invocations, type);
         };

         constexpr auto rm1 = strong::resource::id{ 1};
         constexpr auto rm2 = strong::resource::id{ 2};

         // configure the local rm:s
         common::transaction::context().configure( { 
            { "rm-mockup", "rm1", &casual_mockup_xa_switch_static},
            { "rm-mockup", "rm2", &casual_mockup_xa_switch_static}});

         EXPECT_TRUE( count_invocation( rm1, common::unittest::rm::State::Invoke::xa_open_entry) == 1);
         EXPECT_TRUE( count_invocation( rm2, common::unittest::rm::State::Invoke::xa_open_entry) == 1);
         EXPECT_TRUE( count_invocation( rm1, common::unittest::rm::State::Invoke::xa_start_entry) == 0);
         EXPECT_TRUE( count_invocation( rm2, common::unittest::rm::State::Invoke::xa_start_entry) == 0);

         // begin transaction
         {
            EXPECT_TRUE( local::begin() == common::code::tx::ok);
            EXPECT_TRUE( count_invocation( rm1, common::unittest::rm::State::Invoke::xa_start_entry) == 1);
            EXPECT_TRUE( count_invocation( rm2, common::unittest::rm::State::Invoke::xa_start_entry) == 1);
         }

         // commit transaction
         {
            EXPECT_TRUE( local::commit() == common::code::tx::ok);
            EXPECT_TRUE( count_invocation( rm1, common::unittest::rm::State::Invoke::xa_end_entry) == 1);
            EXPECT_TRUE( count_invocation( rm2, common::unittest::rm::State::Invoke::xa_end_entry) == 1);

            auto resource_proxy_invoked = []( auto& state, auto id) -> decltype( state.resources.at( 0))
            {
               if( auto found = common::algorithm::find( state.resources, id))
                  return found.front();

               common::code::raise::error( common::code::casual::invalid_argument, "failed to find ", id);
            };

            auto state = unittest::state();
            auto& state_rm1 = resource_proxy_invoked( state, rm1);
            auto& state_rm2 = resource_proxy_invoked( state, rm2);
            EXPECT_TRUE( state_rm1.instances.at( 0).metrics.resource.count == 2) << CASUAL_NAMED_VALUE( state_rm1);
            EXPECT_TRUE( state_rm2.instances.at( 0).metrics.resource.count == 2) << CASUAL_NAMED_VALUE( state_rm2);
         }

         // unittest only...
         common::transaction::context().clear();
      }

      TEST( transaction_manager, two_resources__send_prepare_request__one_external_resource__send_one_phase_commit__expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         local::involved::Process rm1;
         local::involved::Process rm2;

         auto trid = common::transaction::id::create( process::handle());
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
            message.process = process::handle();

            local::send::tm( message);
         }

         // resource 1 receives a resource::prepare::Request from transaction manager
         // and sends a resource::prepare::Reply xa::ok back to transaction manager
         {
            common::message::transaction::resource::prepare::Request message;
            communication::device::blocking::receive( rm1.inbound, message);

            auto reply = common::message::reverse::type( message);

            reply.resource = message.resource;
            reply.state = common::code::xa::ok;
            reply.trid = message.trid;

            local::send::tm( reply);
         }

         {
            // resource 2 (outbound) receives a resource::prepare::Request from transaction manager
            common::message::transaction::resource::prepare::Request message;
            communication::device::blocking::receive( rm2.inbound, message);

            // act as another domains transaction manager that got a resource::prepare::Request
            // and send a resource::commit::Request (one-phase-optimization) to the first transaction manager
            {
               common::message::transaction::resource::commit::Request message;
               message.trid = branch;
               message.process = process::handle();
               message.flags = common::flag::xa::Flag::one_phase;

               local::send::tm( message);
            }

            // receive resource::commit::Reply from transaction manager
            {
               auto message = communication::ipc::receive< common::message::transaction::resource::commit::Reply>();

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
            auto message = communication::ipc::receive< common::message::transaction::commit::Reply>();

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

         auto trid = common::transaction::id::create( process::handle());
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
            message.process = process::handle();

            local::send::tm( message);
         }

         // resource 1 receives a resource::prepare::Request from transaction manager
         // and sends a resource::prepare::Reply xa::ok back to transaction manager
         {
            common::message::transaction::resource::prepare::Request message;
            communication::device::blocking::receive( rm1.inbound, message);

            auto reply = common::message::reverse::type( message);

            reply.resource = message.resource;
            reply.state = common::code::xa::ok;
            reply.trid = message.trid;

            local::send::tm( reply);
         }

         {
            // resource 2 (outbound) receives a resource::prepare::Request from transaction manager
            common::message::transaction::resource::prepare::Request message;
            communication::device::blocking::receive( rm2.inbound, message);

            // act as another domains transaction manager that got a resource::prepare::Request
            // and send a resource::prepare::Request to the first transaction manager
            {
               common::message::transaction::resource::prepare::Request message;
               message.trid = branch;
               message.process = process::handle();

               local::send::tm( message);
            }

            // receive resource::prepare::Reply from transaction manager
            {
               auto message = communication::ipc::receive< common::message::transaction::resource::prepare::Reply>();

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
            auto message = communication::ipc::receive< common::message::transaction::commit::Reply>();

            EXPECT_TRUE( message.trid == trid) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.state == decltype( message.state)::ok) << CASUAL_NAMED_VALUE( message.state);
         }

      }

      TEST( transaction_manager, one_resources__send_commit_request__one_external_resource__send_prepare_request__expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::system, local::configuration::base);

         local::involved::Process rm1;

         auto trid = common::transaction::id::create( process::handle());

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
            message.process = process::handle();

            local::send::tm( message);
         }

         {
            // resource 1 (outbound) receives a resource::commit::Request from transaction manager
            common::message::transaction::resource::commit::Request message;
            communication::device::blocking::receive( rm1.inbound, message);

            // act as another domains transaction manager that got a resource::prepare::Request
            // and send a resource::prepare::Request to the first transaction manager
            {
               common::message::transaction::resource::prepare::Request message;
               message.trid = trid;
               message.process = process::handle();

               local::send::tm( message);
            }

            // receive resource::prepare::Reply from transaction manager
            {
               auto message = communication::ipc::receive< common::message::transaction::resource::prepare::Reply>();

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
            auto message = communication::ipc::receive< common::message::transaction::commit::Reply>();

            EXPECT_TRUE( message.trid == trid) << CASUAL_NAMED_VALUE( message);
            EXPECT_TRUE( message.state == decltype( message.state)::ok) << CASUAL_NAMED_VALUE( message.state);
         }

      }
   
   } // transaction

} // casual
