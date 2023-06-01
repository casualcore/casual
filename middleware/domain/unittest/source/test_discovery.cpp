//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "domain/discovery/api.h"
#include "domain/unittest/manager.h"
#include "domain/discovery/instance.h"

#include "common/communication/device.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/array.h"

namespace casual
{
   using namespace std::literals;
   using namespace common;

   namespace domain::discovery
   {

      namespace local
      {
         namespace
         {
            auto& device()
            {
               static communication::instance::outbound::detail::optional::Device device{ discovery::instance::identity};
               return device;
            }
         } // <unnamed>
      } // local

      TEST( domain_discovery, no_discoverable__outbound_request___expect_empty_message)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();

         auto correlation = discovery::request( {}, {});

         // wait for the reply
         {
            message::discovery::api::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
         }
      }

      TEST( domain_discovery, no_discoverable__inbound_request___expect_empty_message)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();

         auto correlation = discovery::request( message::discovery::Request{ process::handle()});

         // wait for the reply
         {
            message::discovery::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
         }
      }


      TEST( domain_discovery, register_as_discover_provider__send_api_request___expect_request__then_reply)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();

         // we register our self
         discovery::provider::registration( discovery::provider::Ability::discover);

         auto correlation = discovery::request( { "a"}, {});

         // wait for the request, and send reply reply
         {
            auto request = communication::ipc::receive< message::discovery::Request>();
            auto reply = common::message::reverse::type( request);
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // wait for the reply
         {
            auto reply = communication::ipc::receive< message::discovery::api::Reply>();
            EXPECT_TRUE( reply.correlation == correlation);
         }
      }

      TEST( domain_discovery, register_as_discover_provider__send_api_request___send_rediscover__expect_prospects__then_reply__then_send_rediscover_expect_no_prospects)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();

         // we register our self
         discovery::provider::registration( discovery::provider::Ability::discover);

         auto discovery_request_correlation = discovery::request( { "a"}, {});

         // wait for the request
         auto first_discovery_request = communication::ipc::receive< message::discovery::Request>();

         // hold the reply and send it later
         auto first_discovery_reply = common::message::reverse::type( first_discovery_request);

         EXPECT_TRUE( algorithm::equal( first_discovery_request.content.services, array::make( "a"sv))) << CASUAL_NAMED_VALUE( first_discovery_request.content.services);

         auto rediscover_correlation = discovery::rediscovery::request();

         {
            // receive the discovery request folloing the rediscover request
            auto request = communication::ipc::receive< message::discovery::Request>();
            // check that the in-flight prospect 'a' is in the request.
            EXPECT_TRUE( algorithm::equal( request.content.services, array::make( "a"sv))) << CASUAL_NAMED_VALUE( request.content.services);
            auto reply = common::message::reverse::type( request);
            // send empty reply
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // send empty reply on first request
         communication::device::blocking::send( first_discovery_request.process.ipc, first_discovery_reply);

         // wait for the reply for first discovery request
         {
            auto reply = communication::ipc::receive< message::discovery::api::Reply>();
            EXPECT_TRUE( reply.correlation == discovery_request_correlation);
            EXPECT_TRUE( reply.content.services.empty()) << CASUAL_NAMED_VALUE( reply.content.services);
         }

         // wait for the reply on rediscovery request
         {
            auto reply = communication::ipc::receive< message::discovery::api::rediscovery::Reply>();
            EXPECT_TRUE( reply.correlation == rediscover_correlation);
         }

         {
            // make one more rediscover and now there should not be any prospects in-flight
            auto rediscover_correlation = discovery::rediscovery::request();
            {
               auto request = communication::ipc::receive< message::discovery::Request>();
               EXPECT_TRUE( request.content.services.empty()) << CASUAL_NAMED_VALUE( request.content.services);
               auto reply = common::message::reverse::type( request);
               communication::device::blocking::send( request.process.ipc, reply);
            }
            {
               auto reply = communication::ipc::receive< message::discovery::api::rediscovery::Reply>();
               EXPECT_TRUE( reply.correlation == rediscover_correlation);
            }
         }
      }

      TEST( domain_discovery, register_as_means_provider__send_discovery_request___expect_request__then_reply)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();

         // we register our self
         discovery::provider::registration( discovery::provider::Ability::lookup);

         auto request = message::discovery::Request{ process::handle()};
         request.content.services = { "a"};
         auto correlation = discovery::request( request);

         // wait for the internal request, and send reply reply
         {
            auto request = communication::ipc::receive< message::discovery::lookup::Request>();
            EXPECT_TRUE( algorithm::equal( request.content.services, array::make( "a")));
            auto reply = common::message::reverse::type( request);
            reply.content.services = { { "a", "foo", common::service::transaction::Type::branch, common::service::visibility::Type::discoverable}};
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // wait for the reply
         {
            auto reply = communication::ipc::receive< message::discovery::Reply>();
            EXPECT_TRUE( reply.correlation == correlation);
            ASSERT_TRUE( reply.content.services.size() == 1);
         }
      }


      TEST( domain_discovery, register_as_discover_provider_x2__send_outbound_request___expect_2_request__then_reply)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();

         // we register our self
         discovery::provider::registration( discovery::provider::Ability::discover);

         // we fake our next registration...
         discovery::provider::registration( process::Handle{ strong::process::id{ process::id().value() + 1}, process::handle().ipc}, discovery::provider::Ability::discover);

         auto correlation = discovery::request( { "a"}, {});

         auto handle_request = []()
         {
            auto request = communication::ipc::receive< message::discovery::Request>();
            auto reply = common::message::reverse::type( request);
            communication::device::blocking::send( request.process.ipc, reply);
         };

         // handle both request
         handle_request();
         handle_request();

         // wait for the reply
         {
            message::discovery::api::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
         }
      }

      TEST( domain_discovery, register_as_external_and_internal_discover_provider__send_forward_request___expect_internal_and_external_discovery_requests__then_reply)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();

         // we register our self
         discovery::provider::registration( discovery::provider::Ability::discover);
         discovery::provider::registration( { strong::process::id{ process::id().underlying() + 1}, process::handle().ipc}, discovery::provider::Ability::lookup);

         auto correlation = discovery::request( [](){
            message::discovery::Request request{ process::handle()};
            request.directive = decltype( request.directive)::forward;
            request.content.queues = { "a", "b"};
            return request;
         }());

         {
            // expect lookup
            auto request = communication::ipc::receive< message::discovery::lookup::Request>();
            auto reply = common::message::reverse::type( request);
            reply.content.queues = { { "a"}};
            reply.absent.queues = { "b"};
            communication::device::blocking::send( request.process.ipc, reply);
         }

         {
            // expect a discovery
            auto request = communication::ipc::receive< message::discovery::Request>();
            EXPECT_TRUE( request.content.queues.size() == 1);
            EXPECT_TRUE( request.content.queues.at( 0) == "b");
            
            auto reply = common::message::reverse::type( request); 
            reply.content.queues = { { "a"}};
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // wait for the reply
         {
            auto reply = communication::ipc::receive< message::discovery::Reply>();
            EXPECT_TRUE( reply.correlation == correlation);
            ASSERT_TRUE( reply.content.queues.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.content.queues.at( 0).name == "a");
         }
      }

      TEST( domain_discovery, register_discover_external_internal_and_knows___rediscover___expect_known_and_discover_external_request__then_reply)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();
         using Ability = discovery::provider::Ability;

         discovery::provider::registration( { Ability::discover, Ability::lookup, Ability::fetch_known});

         auto correlation = discovery::rediscovery::request();

         // needs
         {
            auto request = communication::ipc::receive< message::discovery::fetch::known::Request>();
            auto reply = common::message::reverse::type( request, process::handle());
            reply.content.queues = { "a", "b"};
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // discovery external
         {
            auto request = communication::ipc::receive< message::discovery::Request>();
            EXPECT_TRUE( algorithm::equal( request.content.queues, array::make( "a"sv, "b"sv))) << CASUAL_NAMED_VALUE( request.content.queues);
            auto reply = common::message::reverse::type( request);
            reply.content.queues = { { "a"}};
            communication::device::blocking::send( request.process.ipc, reply);
         };


         // wait for the reply
         {
            auto reply = communication::ipc::receive< message::discovery::api::rediscovery::Reply>();
            EXPECT_TRUE( reply.correlation == correlation);
            EXPECT_TRUE( algorithm::equal( reply.content.queues, array::make( "a"sv)));
         }
      }

      TEST( domain_discovery, direct_update__expect__known_request__topology_direct_explore__topology_implicit_update)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();

         communication::select::Directive directive;
         communication::ipc::send::Coordinator multiplex{ directive};

         using Ability = discovery::provider::Ability; 
         discovery::provider::registration( { Ability::discover, Ability::topology, Ability::fetch_known});

         const auto origin = strong::domain::id{ uuid::make()};

         {
            message::discovery::topology::direct::Update update{ process::handle()};
            update.origin = decltype( update.origin){ origin, "aaa"};
            update.configured.services = { "x", "z"};
            update.configured.queues = { "b1", "b2"};
            discovery::topology::direct::update( multiplex, update);

            while( multiplex)
               multiplex.send();
         }

         // known
         {
            auto request = communication::ipc::receive< message::discovery::fetch::known::Request>();
            auto reply = common::message::reverse::type( request, process::handle());
            reply.content.services = { "a", "b"};
            reply.content.queues = { "q1", "q2"};
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // topology direct explore
         {
            auto explore = communication::ipc::receive< message::discovery::topology::direct::Explore >();
            ASSERT_TRUE( explore.domains.size() == 1) << CASUAL_NAMED_VALUE( explore);
            EXPECT_TRUE( explore.domains.at( 0) == origin);
            EXPECT_TRUE( algorithm::equal( explore.content.queues, array::make( "b1"sv, "b2"sv, "q1"sv, "q2"sv))) << CASUAL_NAMED_VALUE( explore.content.queues);
            EXPECT_TRUE( algorithm::equal( explore.content.services, array::make( "a"sv, "b"sv, "x"sv, "z"sv))) << CASUAL_NAMED_VALUE( explore.content.services);
         };

         // we get the implicit topology update
         {
            auto reply = communication::ipc::receive< message::discovery::topology::implicit::Update>();
            ASSERT_TRUE( reply.domains.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.domains.at( 0) == common::domain::identity()) << CASUAL_NAMED_VALUE( reply);
         }
      }

      TEST( domain_discovery, implicit_update__expect__known_request__discovery_explore__topology_implicit_update)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();

         communication::select::Directive directive;
         communication::ipc::send::Coordinator multiplex{ directive};

         auto inbound = communication::ipc::inbound::Device{};

         using Ability = discovery::provider::Ability;
         discovery::provider::registration( { Ability::discover, Ability::lookup, Ability::topology, Ability::fetch_known});

         {
            message::discovery::topology::implicit::Update message;
            message.domains.push_back( common::domain::Identity{ "foo"});

            discovery::topology::implicit::update( multiplex, message);
         }

         while( multiplex)
            multiplex.send();

         // known request
         {
            auto request = communication::ipc::receive< message::discovery::fetch::known::Request>();
            auto reply = common::message::reverse::type( request, process::handle());
            reply.content.services = { "a"};
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // discovery explore 
         {
            auto explore = communication::ipc::receive< message::discovery::topology::direct::Explore>();
            EXPECT_TRUE( algorithm::equal( explore.content.services, array::make( "a"sv)));
         };

         // we get the topology update
         {
            auto reply = communication::ipc::receive< message::discovery::topology::implicit::Update>();
            ASSERT_TRUE( reply.domains.size() == 2);
            EXPECT_TRUE( algorithm::find( reply.domains, "foo"));
         }
      }

      TEST( domain_discovery, discover_lookup_reply_with_absent__expect_discovery_request___expect_discovery_reply)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();


         using Ability = discovery::provider::Ability; 
         discovery::provider::registration( { Ability::discover, Ability::lookup});


         // send request with forward, will trigger request to our self
         auto correlation = discovery::request( [](){
            message::discovery::Request request{ process::handle()};
            request.directive = decltype( request.directive)::forward;
            request.content.services = { "A"};
            return request;
         }());

                  
         {
            // expect lookup, we reply with absent
            auto request = communication::ipc::receive< message::discovery::lookup::Request>();
            auto reply = common::message::reverse::type( request);
            for( auto& name : request.content.services)
               reply.absent.services.push_back( name);
            communication::device::blocking::send( request.process.ipc, reply);
         }

         {
            // expect request from discovery
            auto request = communication::ipc::receive< message::discovery::Request>();
            EXPECT_TRUE( request.content.services.at( 0) == "A");
            auto reply = common::message::reverse::type( request); 
            reply.content.services.emplace_back().name = "A";
            communication::device::blocking::send( request.process.ipc, reply);
         }

         {
            // expect reply from discovery
            auto request = communication::ipc::receive< message::discovery::Reply>();
            EXPECT_TRUE( request.correlation == correlation);
            EXPECT_TRUE( request.content.services.at( 0).name == "A");

         }
      } 

      TEST( domain_discovery, discover_lookup_reply_with_found___expect_NO_discovery_request___expect_discovery_reply)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();


         using Ability = discovery::provider::Ability; 
         discovery::provider::registration( { Ability::discover, Ability::lookup});


         // send request with forward, will trigger request to our self
         auto correlation = discovery::request( [](){
            message::discovery::Request request{ process::handle()};
            request.directive = decltype( request.directive)::forward;
            request.content.services = { "A"};
            return request;
         }());


         {
            // expect lookup, we reply that we can provide the wanted service.
            auto request = communication::ipc::receive< message::discovery::lookup::Request>();
            auto reply = common::message::reverse::type( request);
            for( auto& name : request.content.services)
               reply.content.services.emplace_back().name = name;
            communication::device::blocking::send( request.process.ipc, reply);
         }

         {
            // expect reply from discovery
            auto request = communication::ipc::receive< message::discovery::Reply>();
            EXPECT_TRUE( request.correlation == correlation);
            EXPECT_TRUE( request.content.services.at( 0).name == "A");

            // Note: we don't need to check that we didn't get a discovery::Request, since
            // we wouldn't get the discovery::Reply if discovery was waiting on our reply.

         }
      }

      TEST( domain_discovery, register_as_discover_provider__send_10_discover_request__congest__send_another_10___expect_accumulated_provider_request_total_less_then_20)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager( R"(
domain:
   name: A
   environment:
      variables:
         - { key: CASUAL_DISCOVERY_ACCUMULATE_REQUESTS, value: 10}
         - { key: CASUAL_DISCOVERY_ACCUMULATE_TIMEOUT, value: 10ms}
)");

         // crate a separate inbound for the "caller"
         struct
         {
            communication::ipc::inbound::Device device;
            process::Handle process{ process::id(), device.connector().handle().ipc()};

         } caller;

         // we register our self
         discovery::provider::registration( { discovery::provider::Ability::discover, discovery::provider::Ability::lookup});

         auto lookup_reply_services_as_absent = []()
         {
            auto request = communication::ipc::receive< message::discovery::lookup::Request>();
            auto reply = common::message::reverse::type( request);
            reply.absent.services = std::move( request.content.services);
            communication::device::blocking::send( request.process.ipc, reply);
         };

         auto create_service = []( auto name)
         {
            return common::message::service::concurrent::advertise::Service{
               name,
               "",
               common::service::transaction::Type::none,
               common::service::visibility::Type::discoverable,
            };
         }; 

         message::discovery::Request request{ caller.process};
         request.directive = decltype( request.directive)::forward;
         request.content.services = { "a"};

         algorithm::for_n( 10, [ &]()
         {
            communication::device::blocking::send( local::device(), request);
            lookup_reply_services_as_absent();
         });

         // now we've got 10 in-flight. Send another 10, these will be accumulated to one (at least less than 10, since we've got 10ms)
         algorithm::for_n( 10, [ &]( auto index)
         {
            request.content.services.at( 0) = std::to_string( index);
            communication::device::blocking::send( local::device(), request);
            lookup_reply_services_as_absent();
         });

         // reply to the first 10 as the provider (default inbound)
         algorithm::for_n( 10, [ &]()
         {
            auto request = communication::ipc::receive< message::discovery::Request>();
            auto reply = common::message::reverse::type( request);
            reply.content.services = algorithm::transform( request.content.services, create_service);
            
            communication::device::blocking::send( request.process.ipc, reply);
         });

         // there should be less then 10 request.
         {
            platform::size::type count = 0;
            std::vector< std::string> services;

            while( ! algorithm::equal( algorithm::sort( services), array::make( "0", "1", "2", "3", "4", "5", "6", "7", "8", "9")))
            {
               auto request = communication::ipc::receive< message::discovery::Request>();

               algorithm::append( request.content.services, services);

               // we exclude service "3" to the reply
               algorithm::container::erase( request.content.services, "3");

               auto reply = common::message::reverse::type( request);
               reply.content.services = algorithm::transform( request.content.services, create_service);
               
               communication::device::blocking::send( request.process.ipc, reply);
               ++count;
            }

            EXPECT_TRUE( count < 10) << "count: " << count;
         }

         // Ok, now we should get our 20 replies as caller. We start with the first 10
         algorithm::for_n( 10, [ &]()
         {
            auto reply = communication::device::receive< message::discovery::Reply>( caller.device);
            ASSERT_TRUE( reply.content.services.size() == 1) << CASUAL_NAMED_VALUE( reply.content.services);
            EXPECT_TRUE( reply.content.services.at( 0).name == "a");
         });

         {
            std::vector< std::string> services;
            algorithm::for_n( 10, [ &]()
            {
               auto reply = communication::device::receive< message::discovery::Reply>( caller.device);
               algorithm::transform( reply.content.services, std::back_inserter( services), []( auto& service){ return service.name;});
               algorithm::container::sort::unique( services);
            });
            
            // expect all "number services" 0 through 9, without the "non existent" service "3"
            EXPECT_TRUE( algorithm::equal( services, array::make( "0", "1", "2", "4", "5", "6", "7", "8", "9"))) << CASUAL_NAMED_VALUE( services);
         }
      }

   } // domain::discovery

} // casual