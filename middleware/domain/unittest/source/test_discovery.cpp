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

            struct Provider
            {
               Provider( strong::process::id pid)
                  : process{ pid, device.connector().handle().ipc()}
               {};

               Provider() : Provider{ process::id()}
               {}
               
               communication::ipc::inbound::Device device;
               process::Handle process;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( device);
                  CASUAL_SERIALIZE( process);
               )
            };


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

         // wait for the reply for first discovery
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

         communication::ipc::inbound::Device provider1;
         communication::ipc::inbound::Device provider2;

         // we register the two providers
         discovery::provider::registration( provider1, discovery::provider::Ability::discover);
         discovery::provider::registration( provider2, discovery::provider::Ability::discover);

         auto correlation = discovery::request( { "a"}, {});

         auto handle_request = []( auto& device)
         {
            auto request = communication::device::receive< message::discovery::Request>( device);
            auto reply = common::message::reverse::type( request);
            communication::device::blocking::send( request.process.ipc, reply);
         };

         // handle both request
         handle_request( provider1);
         handle_request( provider2);

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

         communication::ipc::inbound::Device provider1;
         communication::ipc::inbound::Device provider2;

         discovery::provider::registration( provider1, discovery::provider::Ability::discover);
         discovery::provider::registration( provider2, discovery::provider::Ability::lookup);

         // send the request
         auto correlation = discovery::request( [](){
            message::discovery::Request request{ process::handle()};
            request.directive = decltype( request.directive)::forward;
            request.content.queues = { "a", "b"};
            return request;
         }());

         {
            // expect lookup
            auto request = communication::device::receive< message::discovery::lookup::Request>( provider2);
            auto reply = common::message::reverse::type( request);
            reply.content.queues = { message::discovery::reply::content::Queue{ "a"}};
            reply.absent.queues = { "b"};
            communication::device::blocking::send( request.process.ipc, reply);
         }

         {
            // expect a discovery
            auto request = communication::device::receive< message::discovery::Request>( provider1);
            EXPECT_TRUE( request.content.queues.size() == 1) << CASUAL_NAMED_VALUE( request.content);
            EXPECT_TRUE( request.content.queues.at( 0) == "b");
            
            auto reply = common::message::reverse::type( request); 
            reply.content.queues = { message::discovery::reply::content::Queue{ "a"}};
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

         discovery::provider::registration( Ability::discover | Ability::lookup | Ability::fetch_known);

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
            reply.content.queues = { message::discovery::reply::content::Queue{ "a"}};
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
         discovery::provider::registration( Ability::discover | Ability::topology | Ability::fetch_known);

         {
            message::discovery::topology::direct::Update update{ process::handle()};
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
            EXPECT_TRUE( algorithm::equal( explore.content.queues, array::make( "b1"sv, "b2"sv, "q1"sv, "q2"sv))) << CASUAL_NAMED_VALUE( explore.content.queues);
            EXPECT_TRUE( algorithm::equal( explore.content.services, array::make( "a"sv, "b"sv, "x"sv, "z"sv))) << CASUAL_NAMED_VALUE( explore.content.services);
         };

         // we get the implicit topology update
         {
            auto reply = communication::ipc::receive< message::discovery::topology::implicit::Update>();
            ASSERT_TRUE( reply.domains.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.domains.at( 0) == common::domain::identity()) << CASUAL_NAMED_VALUE( reply) << '\n' << CASUAL_NAMED_VALUE( common::domain::identity());
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
         discovery::provider::registration( Ability::discover | Ability::lookup | Ability::topology | Ability::fetch_known);

         {
            message::discovery::topology::implicit::Update message{ process::handle()};
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
         discovery::provider::registration( Ability::discover | Ability::lookup);


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
         discovery::provider::registration( Ability::discover | Ability::lookup);


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


         // we register our self
         discovery::provider::registration( discovery::provider::Ability::discover | discovery::provider::Ability::lookup);

         auto lookup_reply_resources_as_absent = []()
         {
            auto request = communication::ipc::receive< message::discovery::lookup::Request>();
            auto reply = common::message::reverse::type( request);
            reply.absent.services = std::move( request.content.services);
            reply.absent.queues = std::move( request.content.queues);
            communication::device::blocking::send( request.process.ipc, reply);
         };

         constexpr static auto create_service = []( auto name)
         {
            return common::message::service::concurrent::advertise::Service{
               name,
               "",
               common::service::transaction::Type::none,
               common::service::visibility::Type::discoverable,
            };
         }; 

         constexpr static auto create_queue = []( auto name)
         {
            return message::discovery::reply::content::Queue{ name};
         };

         auto create_expected_resources = []( auto prefix, auto count)
         {
            std::vector< std::string> result;
            algorithm::for_n( count, [ &result, prefix]( auto index)
            {
               result.push_back( string::compose( prefix, index));
            });
            return result;
         };

         // Ok, lets start sending stuff.

         static auto create_request = []()
         {  
            message::discovery::Request request{ process::handle()};
            request.directive = decltype( request.directive)::forward;
            return request;
         };

         // send 10 request for 'a'
         auto requests = algorithm::generate_n< 10>( [ &]()
         {
            auto request = create_request();
            request.content.services = { "a"};

            request.correlation = communication::device::blocking::send( local::device(), request);
            lookup_reply_resources_as_absent();
            return request;
         });

         auto expected_services = create_expected_resources( "s", 10);
         auto expected_queues = create_expected_resources( "q", 10);


         // now we've got 10 in-flight. Send another 10 each for services and queues, these will be accumulated 
         // to one (at least less than 10, since we've got 10ms)
         requests = algorithm::accumulate( expected_services, std::move( requests), [&]( auto result, auto& service)
         {
            auto request = create_request();
            request.content.services.push_back( service);
            request.correlation = communication::device::blocking::send( local::device(), request);
            lookup_reply_resources_as_absent();
            result.push_back( std::move( request));
            return result;
         });

         requests = algorithm::accumulate( expected_queues, std::move( requests), [&]( auto result, auto& queue)
         {
            auto request = create_request();
            request.content.queues.push_back( queue);
            request.correlation = communication::device::blocking::send( local::device(), request);
            lookup_reply_resources_as_absent();
            result.push_back( std::move( request));
            return result;
         });

         // now we've got 30 requests in flight, an we (as a provider) has not yet answer any discovery
         // requests from discovery. The problem is, we don't really know how many requests discovery will
         // send. It'll be less than 30, but we can't know exactly.
         // We have to do non-blocking receive for both our known 30 discovery_replies (that we've sent 
         // requests for) and our unknown count of discovery_requests that we need to reply to (as a provider)


         platform::size::type accumulated_requests_received{};

         for( const auto& request : requests)
         {
            auto reply = communication::ipc::non::blocking::receive< message::discovery::Reply>( request.correlation);

            while( ! reply)
            {
               if( auto in_request = communication::ipc::non::blocking::receive< message::discovery::Request>())
               {
                  ++accumulated_requests_received;

                  auto in_reply = common::message::reverse::type( *in_request);
                  in_reply.content.services = algorithm::transform( in_request->content.services, create_service);
                  in_reply.content.queues = algorithm::transform( in_request->content.queues, create_queue);
                  communication::device::blocking::send( in_request->process.ipc, in_reply);
               }

               // we sleep for a while to not go bananaz (in log)
               process::sleep( std::chrono::milliseconds{ 2});
               reply = communication::ipc::non::blocking::receive< message::discovery::Reply>( request.correlation);
            }

            EXPECT_TRUE( algorithm::equal( reply->content.services, request.content.services));
            EXPECT_TRUE( algorithm::equal( reply->content.queues, request.content.queues));
         }

         EXPECT_TRUE( accumulated_requests_received < 30);
      }

      TEST( domain_discovery, discover_provider_10__discover__reply_to_one__expect_short_circuit_and_reply)
      {
         common::unittest::Trace trace;

         auto domain = unittest::manager();

         std::array< local::Provider, 10> providers;

         for( auto& provider : providers)
            discovery::provider::registration( provider.device, discovery::provider::Ability::discover);

         // send one request to discovery
         {
            message::discovery::Request request{ process::handle()};
            request.directive = decltype( request.directive)::forward;
            request.content.services = { "a", "b"};
            discovery::request( request);
         }

         // discovery will ask all 10 of our register providers, but we'll only reply from one
         {
            auto request = communication::device::receive< message::discovery::Request>( providers[ 5].device);
            auto reply = common::message::reverse::type( request);
            reply.content.services = algorithm::transform( request.content.services, []( auto& name)
            {
               return common::message::service::concurrent::advertise::Service{
                  name,
                  "",
                  common::service::transaction::Type::none,
                  common::service::visibility::Type::discoverable
               };
            });

            communication::device::blocking::send( request.process.ipc, reply);
         }

         // we expect a reply to our original request
         {
            auto reply = communication::ipc::receive< message::discovery::Reply>();
            ASSERT_TRUE( reply.content.services.size() == 2);
            EXPECT_TRUE( reply.content.services.at( 0).name == "a");
            EXPECT_TRUE( reply.content.services.at( 1).name == "b");
         }

      }



      TEST( domain_discovery, act_as_SM_GW__discover_q1_s2_q1_q2__s1_q1_is_known___extended_discovery_for_s2_q2__all_is_found___expect_s1_s2_q1_q2__in_reply)
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
         local::Provider caller;

         // we register our self
         discovery::provider::registration( discovery::provider::Ability::discover | discovery::provider::Ability::lookup);


         constexpr static auto create_service = []( auto name)
         {
            return common::message::service::concurrent::advertise::Service{
               name,
               "",
               common::service::transaction::Type::none,
               common::service::visibility::Type::discoverable,
            };
         }; 

         constexpr static auto create_queue = []( auto name)
         {
            return message::discovery::reply::content::Queue{ name};
         };

         // will reply with absent s2, q2, known s1, q1
         constexpr static auto lookup_reply_resources = []()
         {
            auto request = communication::ipc::receive< message::discovery::lookup::Request>();
            auto reply = common::message::reverse::type( request);

            EXPECT_TRUE( algorithm::equal( request.content.services, array::make( "s1", "s2")));
            EXPECT_TRUE( algorithm::equal( request.content.queues, array::make( "q1", "q2"))) << CASUAL_NAMED_VALUE( request.content.queues);

            reply.content.services.push_back( create_service( "s1"));
            reply.absent.services.emplace_back( "s2");

            reply.content.queues.push_back( create_queue( "q1"));
            reply.absent.queues.emplace_back( "q2");
            
            communication::device::blocking::send( request.process.ipc, reply);
         };

         // Ok, lets start sending stuff.

         // send request with s1, s2, q1, q2
         {
            message::discovery::Request request{ caller.process};
            request.directive = decltype( request.directive)::forward;
            request.content.services = { "s1", "s2"};
            request.content.queues = { "q1", "q2"};
            
            communication::device::blocking::send( local::device(), request);

            // reply as "service/queue manager"
            lookup_reply_resources();
         };

         // reply as "gateway manager"
         {
            auto request = communication::ipc::receive< message::discovery::Request>();
            auto reply = common::message::reverse::type( request);
            reply.content.services = algorithm::transform( request.content.services, create_service);
            reply.content.queues = algorithm::transform( request.content.queues, create_queue);
            
            communication::device::blocking::send( request.process.ipc, reply);
         };

         // get the reply
         {
            auto equal_name = []( auto& lhs, auto& rhs){ return lhs.name == rhs;};

            auto reply = communication::device::receive< message::discovery::Reply>( caller.device);
            EXPECT_TRUE( algorithm::equal( reply.content.services, array::make( "s1", "s2"), equal_name));
            EXPECT_TRUE( algorithm::equal( reply.content.queues, array::make( "q1", "q2"), equal_name));
         }

      }

   } // domain::discovery

} // casual