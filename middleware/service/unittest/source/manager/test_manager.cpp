//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"
#include "common/unittest/file.h"

#include "service/manager/admin/server.h"
#include "service/manager/admin/model.h"
#include "service/forward/instance.h"
#include "service/unittest/utility.h"

#include "common/message/domain.h"
#include "common/message/event.h"
#include "common/message/service.h"

#include "common/service/lookup.h"
#include "common/communication/instance.h"
#include "common/event/listen.h"
#include "common/algorithm/container.h"

#include "common/code/xatmi.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/log.h"

#include "domain/discovery/api.h"
#include "domain/unittest/manager.h"
#include "domain/unittest/configuration.h"

#include "configuration/unittest/utility.h"
#include "configuration/model/transform.h"

namespace casual
{
   namespace service
   {
      namespace local
      {
         namespace
         {
            namespace ipc
            {
               auto& inbound() { return common::communication::ipc::inbound::device();}
            } // ipc

            namespace configuration
            {
               constexpr auto base = R"(
domain:
   name: service-domain

   servers:
      - path: bin/casual-service-manager
)";


               template< typename... C>
               auto load( C&&... contents)
               {
                  return casual::configuration::unittest::load( std::forward< C>( contents)...);
               }



            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return domain::unittest::manager( configuration::base, std::forward< C>( configurations)...);
            }

            namespace call
            {
               auto state()
               {
                  return serviceframework::service::protocol::binary::Call{}( 
                     manager::admin::service::name::state).extract< manager::admin::model::State>();
               }

            } // call

            namespace service
            {
               const manager::admin::model::Service* find( const manager::admin::model::State& state, const std::string& name)
               {
                  auto found = common::algorithm::find_if( state.services, [&name]( auto& s){
                     return s.name == name;
                  });

                  if( found) 
                     return &( *found);

                  return nullptr;
               }
            } // service

            namespace instance
            {
               const manager::admin::model::instance::Sequential* find( const manager::admin::model::State& state, common::strong::process::id pid)
               {
                  auto found = common::algorithm::find_if( state.instances.sequential, [pid]( auto& i){
                     return i.process.pid == pid;
                  });

                  if( found) { return &( *found);}

                  return nullptr;
               }
            } // instance

            //! send to service-manager
            template< typename M>
            auto send( M&& message)
            {
               return common::communication::device::blocking::send( 
                  common::communication::instance::outbound::service::manager::device(),
                  std::forward< M>( message));
            }


         } // <unnamed>
      } // local


      TEST( service_manager, startup_shutdown__expect_no_throw)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            auto domain = local::domain();
         });

      }

      TEST( service_manager, configuration_default___expect_same)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto origin = local::configuration::load( local::configuration::base);

         auto model = casual::configuration::model::transform( casual::domain::unittest::configuration::get());

         EXPECT_TRUE( origin.service == model.service) << CASUAL_NAMED_VALUE( origin) << '\n' << CASUAL_NAMED_VALUE( model);
      }

      TEST( service_manager, configuration_routes___expect_same)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      -  name: a
         routes: [ b, c, d]
         execution:
            timeout: 
               duration: 10ms
               contract: kill



)";

         auto domain = local::domain( configuration);

         auto origin = local::configuration::load( local::configuration::base, configuration);

         auto model = casual::configuration::model::transform( casual::domain::unittest::configuration::get());

         EXPECT_TRUE( origin.service == model.service) << CASUAL_NAMED_VALUE( origin.service) << '\n' << CASUAL_NAMED_VALUE( model.service);
      }

      TEST( service_manager, configuration_post)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   services:
      -  name: a
         routes: [ b, c, d]
         execution:
            timeout: 
               duration: 10ms
               contract: kill

      -  name: modify
         routes: [ x, y]
         execution:
            timeout: 
               duration: 10ms
               contract: kill

)");


         auto wanted = local::configuration::load( local::configuration::base, R"(
domain:
   services:
      -  name: b
         routes: [ e, f, g]
         execution:
            timeout: 
               duration: 20ms
               contract: linger

      -  name: modify
         routes: [ x, z]
         execution:
            timeout: 
               duration: 20ms
               contract: linger

)");

         // make sure the wanted differs (otherwise we're not testing anything...)
         ASSERT_TRUE( wanted.service != casual::configuration::model::transform( casual::domain::unittest::configuration::get()).service);

         // post the wanted model (in transformed user representation)
         auto updated = casual::configuration::model::transform( 
            casual::domain::unittest::configuration::post( casual::configuration::model::transform( wanted)));

         EXPECT_TRUE( wanted.service == updated.service) << " " << CASUAL_NAMED_VALUE( wanted.service) << '\n' << CASUAL_NAMED_VALUE( updated.service);

      }


      namespace local
      {
         namespace
         {
            bool has_services( const std::vector< manager::admin::model::Service>& services, std::initializer_list< const char*> wanted)
            {
               return common::algorithm::all_of( wanted, [&services]( auto& s){
                  return common::algorithm::find_if( services, [&s]( auto& service){
                     return service.name == s;
                  });
               });
            }

         } // <unnamed>
      } // local

      

      TEST( service_manager, startup___expect_1_service_in_state)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto state = local::call::state();

         EXPECT_TRUE( local::has_services( state.services, { manager::admin::service::name::state}));
         EXPECT_FALSE( local::has_services( state.services, { "non/existent/service"}));
      }


      TEST( service_manager, advertise_2_services_for_1_server__expect__3_services)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         auto state = local::call::state();

         ASSERT_TRUE( local::has_services( state.services, { manager::admin::service::name::state, "service1", "service2"}));
         
         {
            auto service = local::service::find( state, "service1");
            ASSERT_TRUE( service);
            ASSERT_TRUE( service->instances.sequential.size() == 1);
            EXPECT_TRUE( service->instances.sequential.at( 0).process == common::process::handle());
         }
         {
            auto service = local::service::find( state, "service2");
            ASSERT_TRUE( service);
            ASSERT_TRUE( service->instances.sequential.size() == 1);
            EXPECT_TRUE( service->instances.sequential.at( 0).process == common::process::handle());
         }
      }

      TEST( service_manager, advertise_A__route_B_A__expect_lookup_for_B)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      - name: A
        routes: [ B]

)";

         auto domain = local::domain( configuration);

         service::unittest::advertise( { "A"});

         auto state = local::call::state();

         auto service = common::service::lookup::reply( common::service::Lookup{ "B"});
         // we expect the 'real-name' of the service to be replied
         EXPECT_TRUE( service.service.name == "A");


         EXPECT_TRUE( local::has_services( state.services, { "B"})) << CASUAL_NAMED_VALUE( state.services);
         EXPECT_TRUE( ! local::has_services( state.services, { "A"})) << CASUAL_NAMED_VALUE( state.services);
         
      }

      TEST( service_manager, service_a_execution_timeout_duration_1ms__advertise_a__lookup_a____expect__TPETIME__and_assassination_event)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      -  name: a
         execution:
            timeout:
               duration: 1ms

)";

         auto domain = local::domain( configuration);

         // setup subscription to verify event
         common::event::subscribe( common::process::handle(), { common::message::event::process::Assassination::type()});

         service::unittest::advertise( { "a"});

         auto service = common::service::lookup::reply( common::service::Lookup{ "a"});
         ASSERT_TRUE( ! service.absent());

         auto reply = common::communication::ipc::receive< common::message::service::call::Reply>();

         EXPECT_TRUE( reply.code.result == decltype( reply.code.result)::timeout);

         auto event = common::communication::ipc::receive< common::message::event::process::Assassination>();

         EXPECT_TRUE( event.target == common::process::id());
         EXPECT_TRUE( event.contract == decltype( event.contract)::linger);
         EXPECT_TRUE( event.announcement == "service a timed out");
      }

      TEST( service_manager, service_a_execution_timeout_duration_1ms__advertise_a__reserve_a__lookup_a__expect_lookup_timeout)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      -  name: a
         execution:
            timeout:
               duration: 1ms

)";

         auto domain = local::domain( configuration);

         service::unittest::advertise( { "a"});

         // reserve
         {
            common::service::lookup::reply( common::service::Lookup{ "a"});
         }

         EXPECT_CODE( 
            common::service::lookup::reply( common::service::Lookup{ "a"});
         ,common::code::xatmi::timeout);

         // the first reserve should give timeout reply
         auto reply = common::communication::ipc::receive< common::message::service::call::Reply>();
         EXPECT_TRUE( reply.code.result == decltype( reply.code.result)::timeout);
      }


      TEST( service_manager, env_variables__advertise_A__route_B_A__expect_lookup_for_B)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   default:
      environment:
         variables:
            - key: SA
              value: A
            - key: SB
              value: B
   services:
      - name: ${SA}
        routes: [ "${SB}"]

)";

         auto domain = local::domain( configuration);

         service::unittest::advertise( { "A"});

         auto state = local::call::state();


         EXPECT_TRUE( local::has_services( state.services, { "B"})) << CASUAL_NAMED_VALUE( state.services);
         EXPECT_TRUE( ! local::has_services( state.services, { "A"})) << CASUAL_NAMED_VALUE( state.services);
         
      }


      TEST( service_manager, advertise_A__route_B___expect_discovery_for_B)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      - name: A
        routes: [ B]

)";

         auto domain = local::domain( configuration);
         service::unittest::advertise( { "A"});

         // discover
         {            
            domain::message::discovery::Request request{ common::process::handle()};
            request.content.services = { "B"};
            domain::discovery::request( request);
            auto reply = common::communication::ipc::receive< domain::message::discovery::Reply>();

            ASSERT_TRUE( reply.content.services.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.content.services.at( 0).name == "B") << CASUAL_NAMED_VALUE( reply);
         }
      }

      TEST( service_manager, advertise_a_b__b_undiscoverable__discover_a_b__expect_discover_a)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      -  name: a
         visibility: discoverable
      -  name: b
         visibility: undiscoverable
)";

         auto domain = local::domain( configuration);
         service::unittest::advertise( { "a", "b"});

         // discover
         {            
            domain::message::discovery::Request request{ common::process::handle()};
            request.content.services = { "a", "b"};
            domain::discovery::request( request);
            auto reply = common::communication::ipc::receive< domain::message::discovery::Reply>();

            ASSERT_TRUE( reply.content.services.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.content.services.at( 0).name == "a") << CASUAL_NAMED_VALUE( reply);
         }
      }

      TEST( service_manager, advertise_a__route_a_b__undiscoverable___discover_a_b__expect_no_discover)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   services:
      -  name: a
         routes: [ a, b]
         visibility: undiscoverable
)";

         auto domain = local::domain( configuration);
         service::unittest::advertise( { "a"});

         // discover
         {            
            domain::message::discovery::Request request{ common::process::handle()};
            request.content.services = { "a", "b"};
            domain::discovery::request( request);
            auto reply = common::communication::ipc::receive< domain::message::discovery::Reply>();

            EXPECT_TRUE( reply.content.services.empty()) << CASUAL_NAMED_VALUE( reply);
         }
      }

      TEST( service_manager, advertise_2_services_for_1_server)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         auto state = local::call::state();
         EXPECT_TRUE( local::has_services( state.services, { "service1", "service2"})) << CASUAL_NAMED_VALUE( state.services);

         {
            auto instance = local::instance::find( state, common::process::id());
            ASSERT_TRUE( instance);
            EXPECT_TRUE( instance->state == decltype( instance->state)::idle) << CASUAL_NAMED_VALUE( *instance);
         }
      }


      TEST( service_manager, advertise_2_services_for_1_server__prepare_shutdown___expect_instance_removed_from_advertised_services)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});


         {
            auto state = local::call::state();
            auto service = local::service::find( state, "service1");
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.at( 0).process == common::process::handle());
         }

         {
            common::message::domain::process::prepare::shutdown::Request request;
            request.process = common::process::handle();
            request.processes.push_back( common::process::handle());

            auto reply = common::communication::ipc::call( common::communication::instance::outbound::service::manager::device(), request);

            EXPECT_TRUE( request.processes == reply.processes);
         }

         {
            auto state = local::call::state();
            auto service = local::service::find( state, "service1");
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.empty());
         }
      }


      TEST( service_manager, advertise_2_services_for_1_server__prepare_shutdown__lookup___expect_absent)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         {
            common::message::domain::process::prepare::shutdown::Request request;
            request.process = common::process::handle();
            request.processes.push_back( common::process::handle());

            auto reply = common::communication::ipc::call( common::communication::instance::outbound::service::manager::device(), request);

            EXPECT_TRUE( request.processes == reply.processes);
         }

         EXPECT_CODE({
            auto service = common::service::lookup::reply( common::service::Lookup{ "service1"});
         }, common::code::xatmi::no_entry);

         EXPECT_CODE({
            auto service = common::service::lookup::reply( common::service::Lookup{ "service2"});
         }, common::code::xatmi::no_entry);
      }

      TEST( service_manager, lookup_service__expect_service_to_be_busy)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         {
            auto service = common::service::lookup::reply( common::service::Lookup{ "service1"});
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == common::process::handle());
            EXPECT_TRUE( service.state == decltype( service.state)::idle);
         }

         {
            // we only have one instance, we expect lookup to not get "the reply"
            auto lookup = common::service::Lookup{ "service2"};
            EXPECT_TRUE( ! common::service::lookup::non::blocking::reply( lookup));
         }
      }

      TEST( service_manager, unadvertise_service)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});
         service::unittest::unadvertise( { "service2"});

         // echo server has unadvertise this service. The service is
         // still "present" in service-manager with no instances. Hence it's absent
         EXPECT_CODE({
            common::service::lookup::reply( common::service::Lookup{ "service2"});
         }, common::code::xatmi::no_entry);
      }



      TEST( service_manager, service_lookup_non_existent__expect_absent_reply)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         EXPECT_CODE({
            common::service::lookup::reply( common::service::Lookup{ "non-existent-service"});
         }, common::code::xatmi::no_entry);
      }


      TEST( service_manager, service_lookup_service1__expect__busy_reply__send_ack____expect__idle_reply)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         {
            auto service = common::service::lookup::reply( common::service::Lookup{ "service1"});
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == common::process::handle());
            EXPECT_TRUE( service.state == decltype( service.state)::idle);
         }

         common::service::Lookup lookup{ "service2"};
         {
            // we only have one instance, we expect this to be busy
            EXPECT_TRUE( ! common::service::lookup::non::blocking::reply( lookup));
         }

         {
            // Send Ack
            {
               common::message::service::call::ACK message;
               message.metric.process = common::process::handle();

               common::communication::device::blocking::send( 
                  common::communication::instance::outbound::service::manager::device(),
                  message);
            }

            // get next pending reply, we force it...
            auto service = common::service::lookup::reply( std::move( lookup));

            EXPECT_TRUE( service.state == decltype( service.state)::idle);
         }
      }


      TEST( service_manager, service_lookup_service1__service_lookup_service1_forward_context____expect_forward_reply)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1"});

         auto forward = common::communication::instance::fetch::handle( forward::instance::identity.id);

         {
            auto service = common::service::lookup::reply( common::service::Lookup{ "service1"});
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == common::process::handle());
            EXPECT_TRUE( service.state == decltype( service.state)::idle);
         }


         {
            auto service = common::service::lookup::reply( common::service::Lookup{ "service1", decltype( common::service::lookup::Context::semantic)::no_reply});
            EXPECT_TRUE( service.service.name == "service1");

            // service-manager will let us think that the service is idle, and send us the process-handle to the forward-cache
            EXPECT_TRUE( service.state == decltype( service.state)::idle);
            EXPECT_TRUE( service.process);
            EXPECT_TRUE( service.process == forward);
         }
      }

      TEST( service_manager, service_lookup_service1__server_terminate__expect__service_error_reply)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         auto correlation = []()
         {
            auto service = common::service::lookup::reply( common::service::Lookup{ "service1"});
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == common::process::handle());
            EXPECT_TRUE( service.state == decltype( service.state)::idle);
            
            return service.correlation;
         }();

         {
            common::message::event::process::Exit message;
            message.state.pid = common::process::id();
            message.state.reason = decltype( message.state.reason)::core;

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::service::manager::device(),
               message);
         }

         {
            // we expect to get a service-call-error-reply
            common::message::service::call::Reply message;
            
            common::communication::device::blocking::receive( 
               common::communication::ipc::inbound::device(),
               message);
            
            EXPECT_TRUE( message.code.result == decltype( message.code.result)::service_error);
            EXPECT_TRUE( message.correlation == correlation);
         }
      }

      TEST( service_manager, advertise_a____prepare_shutdown___expect_reply_with_our_self)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "a"});

         // send prepare shutdown
         {
            common::message::domain::process::prepare::shutdown::Request request{ common::process::handle()};
            request.processes.push_back( common::process::handle());
            common::communication::device::blocking::send(
               common::communication::instance::outbound::service::manager::device(),
               request);
         }

         {
            // we expect to get a prepare shutdown reply, with our self
            common::message::domain::process::prepare::shutdown::Reply reply;
            common::communication::device::blocking::receive( local::ipc::inbound(), reply);
            ASSERT_TRUE( reply.processes.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.processes.at( 0) == common::process::handle());
         }

         {
            // we expect service 'a' to be absent.
            auto lookup = common::service::Lookup{ "a"};
            EXPECT_CODE( common::service::lookup::reply( std::move( lookup)), common::code::xatmi::no_entry);
         }
      }

      TEST( service_manager, advertise_a__lookup_a___prepare_shutdown___send_ack___expect_reply_with_our_self)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "a"});

         // 'emulate' that a call is in progress (more like consuming the lookup reply...)
         const auto service = common::service::lookup::reply( common::service::Lookup{ "a"});

         EXPECT_TRUE( service.state == decltype( service.state)::idle);

         // send prepare shutdown
         {
            common::message::domain::process::prepare::shutdown::Request request{ common::process::handle()};
            request.processes.push_back( common::process::handle());
            common::communication::device::blocking::send(
               common::communication::instance::outbound::service::manager::device(),
               request);
         }

         // pretend a service call has been done
         service::unittest::send::ack( service);

         {
            // we expect to get a prepare shutdown reply, with our self
            common::message::domain::process::prepare::shutdown::Reply reply;
            common::communication::device::blocking::receive( local::ipc::inbound(), reply);
            ASSERT_TRUE( reply.processes.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.processes.at( 0) == common::process::handle());
         }

         {
            // we expect service 'a' to be absent.
            auto absent = common::service::Lookup{ "a"};
            EXPECT_CODE( common::service::lookup::reply( std::move( absent)), common::code::xatmi::no_entry);
         }
      }

      TEST( service_manager, advertise_a_b_c_d___service_lookup_a_b_c_d___prepare_shutdown__expect_lookup_reply_1_idle__3_absent)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "a", "b", "c", "d"});

         auto idle = common::service::lookup::reply( common::service::Lookup{ "a"});

         auto lookups = common::algorithm::container::emplace::initialize< std::vector< common::service::Lookup>>(  "b", "c", "d");

         EXPECT_TRUE( idle.state == decltype( idle.state)::idle);

         // the pending lookups should be "busy" (in the first round)
         for( auto& lookup : lookups)
            EXPECT_TRUE( ! common::service::lookup::non::blocking::reply( lookup));

         // send prepare shutdown
         {
            common::message::domain::process::prepare::shutdown::Request request{ common::process::handle()};
            request.processes.push_back( common::process::handle());
            common::communication::device::blocking::send(
               common::communication::instance::outbound::service::manager::device(),
               request);
         }

         // We should get lookup reply with 'absent', hence, the pending lookups should throw 'no_entry'
         for( auto& lookup : lookups)
            EXPECT_CODE( common::service::lookup::reply( std::move( lookup)), common::code::xatmi::no_entry);
         
         // we make service-manager think we've died
         {
            common::message::event::process::Exit message;
            message.state.pid = common::process::id();
            message.state.reason = decltype( message.state.reason)::core;

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::service::manager::device(),
               message);
         }

         {
            // we expect to get a service-call-error-reply
            common::message::service::call::Reply message;
            
            common::communication::device::blocking::receive( local::ipc::inbound(), message);
            
            EXPECT_TRUE( message.code.result == decltype( message.code.result)::service_error);
            EXPECT_TRUE( message.correlation == idle.correlation) << CASUAL_NAMED_VALUE( message) << "\n" << CASUAL_NAMED_VALUE( idle);
         }

         {
            // we expect to get a prepare shutdown reply, with no processes since we "died" mid call.
            common::message::domain::process::prepare::shutdown::Reply reply;
            common::communication::device::non::blocking::receive( local::ipc::inbound(), reply);
            EXPECT_TRUE( reply.processes.empty());
         }
      }

      TEST( service_manager, a_route_b__advertise_a__lookup_b_twice__expect_first_idle__send_ack___expect_second_idle)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: route

   services:
      - name: A
        routes: [ B]

)";

         auto domain = local::domain( configuration);

         service::unittest::advertise( { "A"});

         // we reserve (lock) our instance to the 'call', via the route
         struct 
         {
            common::service::Lookup first{ "B"};
            common::service::Lookup second{ "B"};
         } lookup;

         using State =  common::service::lookup::State;

         // 'emulate' that a call is in progress (more like consuming the lookup reply...)
         const auto first = common::service::lookup::reply( std::move( lookup.first));
         EXPECT_TRUE( first.state == State::idle);

         // expect a second "call" to get busy
         EXPECT_TRUE( ! common::service::lookup::non::blocking::reply( lookup.second));

         // pretend the first call has been done
         service::unittest::send::ack( first);

         // expect the second to be idle now.
         EXPECT_TRUE( common::service::lookup::reply( std::move( lookup.second)).state == State::idle);

      }

      TEST( service_manager, service_lookup__send_ack__expect_metric)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "service1", "service2"});

         auto start = platform::time::clock::type::now();
         auto end = start + std::chrono::milliseconds{ 2};

         {
            auto service = common::service::lookup::reply( common::service::Lookup{ "service1"});
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == common::process::handle());
            EXPECT_TRUE( service.state == decltype( service.state)::idle);
         }

         // subscribe to metric event
         common::message::event::service::Calls event;
         common::event::subscribe( common::process::handle(), { event.type()});


         const auto span = common::strong::execution::span::id::generate();
         EXPECT_TRUE( span);

         // Send Ack
         {
            common::message::service::call::ACK message;
            message.metric.process = common::process::handle();
            message.metric.service = "b";
            message.metric.parent.service = "a";
            message.metric.parent.span = span;
            message.metric.start = start;
            message.metric.end = end;
            message.metric.code.result = common::code::xatmi::service_fail;

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::service::manager::device(),
               message);
         }

         // wait for event
         {
            common::communication::device::blocking::receive( 
               common::communication::ipc::inbound::device(),
               event);
            
            ASSERT_TRUE( event.metrics.size() == 1);
            auto& metric = event.metrics.at( 0);
            EXPECT_TRUE( metric.service == "b");
            EXPECT_TRUE( metric.parent.service == "a");
            EXPECT_TRUE( metric.parent.span == span);
            EXPECT_TRUE( metric.process == common::process::handle());
            EXPECT_TRUE( metric.start == start);
            EXPECT_TRUE( metric.end == end);
            EXPECT_TRUE( metric.code.result == common::code::xatmi::service_fail);
         }
      }

      TEST( service_manager, concurrent_advertise_same_instances_3_times_for_same_service___expect_one_instance_for_service)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         common::algorithm::for_n< 3>( []()
         {
            common::message::service::concurrent::Advertise message{ common::process::handle()};
            message.services.add.push_back( { 
               .name = "concurrent_a", 
               .transaction = common::service::transaction::Type::none, 
               .visibility = common::service::visibility::Type::discoverable});
            
            common::communication::device::blocking::send( 
               common::communication::instance::outbound::service::manager::device(),
               message);
         });

         auto state = local::call::state();
         auto service = local::service::find( state, "concurrent_a");

         ASSERT_TRUE( service);
         auto& instances = service->instances.concurrent;
         ASSERT_TRUE( instances.size() == 1) << CASUAL_NAMED_VALUE( instances);
         EXPECT_TRUE( instances.at( 0).process == common::process::handle());
  
      }

      TEST( service_manager, concurrent_service_lookup__absent_service__expect_service_lookup_forget_with_state_discarded)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();


         common::strong::correlation::id correlation;

         // non existent service a, we wait forever.
         {
            common::message::service::lookup::Request message{ common::process::handle()};
            message.requested = "a";
            message.context.semantic = decltype( message.context.semantic)::wait;

            correlation = local::send( message);
         }

         auto create_discard_request = []( auto& correlation)
         {
            common::message::service::lookup::discard::Request message{ common::process::handle()};
            message.reply = true;
            message.requested = "a";
            message.correlation = correlation;
            return message;
         };

         // discard the lookup
         auto reply = common::communication::ipc::call( common::communication::instance::outbound::service::manager::device(), create_discard_request( correlation));

         EXPECT_TRUE( reply.correlation == correlation);
         EXPECT_TRUE( reply.state == decltype( reply.state)::discarded) << trace.compose( "reply: ", reply);
      }

      TEST( service_manager, concurrent_service_lookup__remote_service__expect_service_lookup_forget_with_state_replied)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::concurrent::advertise( { "a", "b"});


         // non existent service a, we wait forever.
         auto create_lookup_request = []()
         {
            common::message::service::lookup::Request message{ common::process::handle()};
            message.requested = "a";
            message.context.semantic = decltype( message.context.semantic)::wait;
            return message;
         };

         auto lookup = common::communication::ipc::call( common::communication::instance::outbound::service::manager::device(), create_lookup_request());

         auto create_discard_request = []( auto& correlation)
         {
            common::message::service::lookup::discard::Request message{ common::process::handle()};
            message.reply = true;
            message.requested = "a";
            message.correlation = correlation;
            return message;
         };

         // discard the lookup
         auto reply = common::communication::ipc::call( 
            common::communication::instance::outbound::service::manager::device(), 
            create_discard_request( lookup.correlation));

         EXPECT_TRUE( reply.correlation == lookup.correlation);
         EXPECT_TRUE( reply.state == decltype( reply.state)::replied) << trace.compose( "reply.state: ", reply.state);
      }

      TEST( service_manager, lookup_reserved_service__SM_receives_concurrent_service_advertisement__expect_idle_when_reservation_finished)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "local-service"});

         // we reserve ourselves
         {
            auto service = common::service::lookup::reply( common::service::Lookup{ "local-service"});
            EXPECT_TRUE( service.service.name == "local-service");
            EXPECT_TRUE( service.state == decltype( service.state)::idle);
         }

         // lookup 'local-service' again
         common::service::Lookup lookup{ "local-service"};

         // some unrelated concurrent services are advertised while our second lookup is pending
         service::unittest::concurrent::advertise( { "some-remote-service", "some-other-remote-service"});

         // we have ourselves reserved - expect to be busy
         EXPECT_TRUE( ! common::service::lookup::non::blocking::reply( lookup));

         {
            // send ack for our initial "call"
            {
               common::message::service::call::ACK message;
               message.metric.process = common::process::handle();

               common::communication::device::blocking::send( 
                  common::communication::instance::outbound::service::manager::device(),
                  message);
            }

            // we should be idle again
            auto service = common::service::lookup::reply( std::move( lookup));
            EXPECT_TRUE( service.state == decltype( service.state)::idle);
         }
      }

      TEST( service_manager, lookup_reserved_service__SM_receives_process_exit_event__expect_idle_when_reservation_finished)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         service::unittest::advertise( { "local-service"});

         // we reserve ourselves
         {
            auto service = common::service::lookup::reply( common::service::Lookup{ "local-service"});
            EXPECT_TRUE( service.service.name == "local-service");
            EXPECT_TRUE( service.state == decltype( service.state)::idle);
         }

         // lookup 'local-service' again
         common::service::Lookup lookup{ "local-service"};

         // meanwhile, some process dies
         {
            common::message::event::process::Exit event;
            event.state.pid = common::strong::process::id{ 42};
            event.state.reason = decltype( event.state.reason)::exited;
            common::communication::device::blocking::send( common::communication::instance::outbound::service::manager::device(), event);
         }

         // we have ourselves reserved - expect to be busy
         EXPECT_TRUE( ! common::service::lookup::non::blocking::reply( lookup));

         {
            // send ack for our initial "call"
            {
               common::message::service::call::ACK message;
               message.metric.process = common::process::handle();

               common::communication::device::blocking::send( 
                  common::communication::instance::outbound::service::manager::device(),
                  message);
            }

            // we should be idle again
            auto service = common::service::lookup::reply( std::move( lookup));
            EXPECT_TRUE( service.state == decltype( service.state)::idle);
         }
      }

   } // service
} // casual
