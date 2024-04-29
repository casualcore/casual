//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/admin/cli.h"

#include "domain/discovery/api.h"
#include "domain/discovery/common.h"
#include "domain/discovery/admin/model.h"
#include "domain/discovery/admin/server.h"
#include "domain/discovery/instance.h"

#include "common/terminal.h"
#include "common/communication/ipc.h"
#include "common/communication/instance.h"
#include "common/serialize/create.h"
#include "common/stream.h"
#include "common/message/counter.h"

#include "casual/cli/state.h"
#include "casual/assert.h"

#include "serviceframework/service/protocol/call.h"


namespace casual
{
   using namespace common;

   namespace domain::discovery::admin
   {
      namespace local
      {
         namespace
         {
            namespace call
            {
               admin::model::State state()
               {
                  return serviceframework::service::protocol::binary::Call{}( 
                     admin::service::name::state).extract< admin::model::State>();
               }

               auto discover( std::vector< std::string> services, std::vector< std::string> queues)
               {
                  return communication::ipc::receive< message::discovery::api::Reply>( discovery::request( std::move( services), std::move( queues)));
               }
            } // call


            namespace rediscovery
            {
               auto option()
               {
                  auto invoke = []()
                  {
                     auto correlation = discovery::rediscovery::request();
                     if( ! terminal::output::directive().block())
                     {
                        communication::ipc::inbound::device().discard( correlation);
                        return;
                     }

                     communication::ipc::receive< message::discovery::api::rediscovery::Reply>( correlation);
                  };

                  return argument::Option{
                     std::move( invoke),
                     { "--rediscover"},
                     "rediscover all 'discoverable' agents"
                  };
               }
            } // rediscovery

            namespace list
            {
               namespace providers
               {
                  auto option()
                  {
                     static constexpr auto create_formatter = []()
                     {
                        auto format_pid = []( auto& provider) { return provider.process.pid;};
                        auto format_abilities = []( auto& provider) { return provider.abilities;};

                        return terminal::format::formatter< model::Provider >::construct(
                           terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
                           terminal::format::column( "abilities", format_abilities, terminal::color::yellow, terminal::format::Align::left)
                        );
                     };

                     auto invoke = []()
                     {
                        auto state = local::call::state();
                        create_formatter().print( std::cout, state.providers);
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "--list-providers"},
                        R"(INCUBATION - list current discovery providers

These are the providers that has registered them self with discovery abilities

@attention INCUBATION - might change during, or in between minor version.
)"
                     };
                  }
               } // providers
            } // list

            namespace discover
            {
               namespace services
               {
                  auto option()
                  {
                     static constexpr auto create_formatter = []()
                     {
                        auto format_name = []( auto& service) { return service.name;};
                        auto format_hops = []( auto& service) { return service.property.hops;};

                        return terminal::format::formatter< message::discovery::reply::Service>::construct(
                           terminal::format::column( "name", format_name, terminal::color::yellow, terminal::format::Align::left),
                           terminal::format::column( "hops", format_hops, terminal::color::white, terminal::format::Align::right)
                        );
                     };

                     auto invoke = []( std::vector< std::string> services)
                     {
                        auto reply = local::call::discover( std::move( services), {});
                        create_formatter().print( std::cout, reply.content.services);
                     };

                     return argument::Option{
                        argument::option::one::many( std::move( invoke)),
                        { "--services"},
                        R"(force discover of provided services

Will try to find provided services in other domains.
)"
                     };
                  }
                  
               } // services

               namespace queues
               {
                  auto option()
                  {
                     static constexpr auto create_formatter = []()
                     {
                        auto format_name = []( auto& queue) { return queue.name;};

                        return terminal::format::formatter< message::discovery::reply::Queue>::construct(
                           terminal::format::column( "name", format_name, terminal::color::yellow, terminal::format::Align::left)
                        );
                     };

                     auto invoke = []( std::vector< std::string> queues)
                     {
                        auto reply = local::call::discover( {}, std::move( queues));
                        create_formatter().print( std::cout, reply.content.queues);
                     };

                     return argument::Option{
                        argument::option::one::many( std::move( invoke)),
                        { "--queues"},
                        R"(force discover of provided queues

Will try to find provided queues in other domains.
)"
                     };
                  }
                  
               } // queues
            } // discover

            namespace metric
            {
               struct Row
               {
                  std::string_view name;
                  platform::size::type completed{};
                  platform::size::type pending{};
               };

               auto transform( const common::message::counter::Reply& message) noexcept
               {
                  std::vector< Row> result;

                  auto get_sent = [ &message]( common::message::Type type) -> platform::size::type
                  {
                     if( auto found = algorithm::find( message.entries, type))
                        return found->sent;
                     return 0;
                  };

                  auto get_received = [ &message]( common::message::Type type) -> platform::size::type
                  {
                     if( auto found = algorithm::find( message.entries, type))
                        return found->received;
                     return 0;
                  };

                  {
                     Row row{ "discovery-out"};
                     row.completed = get_received( common::message::Type::domain_discovery_reply);
                     row.pending = get_sent( common::message::Type::domain_discovery_request) - row.completed;                     
                     result.push_back( row);
                  }

                  {
                     Row row{ "discovery-in"};
                     row.completed = get_sent( common::message::Type::domain_discovery_reply);
                     row.pending = get_received( common::message::Type::domain_discovery_request) - row.completed;                     
                     result.push_back( row);
                  }

                  {
                     Row row{ "lookup"};
                     row.completed = get_received( common::message::Type::domain_discovery_lookup_reply);
                     row.pending = get_sent( common::message::Type::domain_discovery_lookup_request) - row.completed;                     
                     result.push_back( row);
                  }

                  {
                     Row row{ "fetch-known"};
                     row.completed = get_received( common::message::Type::domain_discovery_fetch_known_reply);
                     row.pending = get_sent( common::message::Type::domain_discovery_fetch_known_request) - row.completed;                     
                     result.push_back( row);
                  }

                  return result;
               }

               auto option()
               {
                  static constexpr auto create_formatter = []()
                  {
                     return terminal::format::formatter< metric::Row>::construct(
                        terminal::format::column( "name", std::mem_fn( &Row::name), terminal::color::yellow, terminal::format::Align::left),
                        terminal::format::column( "completed", std::mem_fn( &Row::completed), terminal::color::white, terminal::format::Align::right),
                        terminal::format::column( "pending", std::mem_fn( &Row::pending), terminal::color::white, terminal::format::Align::right)
                     );
                  };

                  auto invoke = []()
                  {
                     communication::instance::outbound::detail::optional::Device device{ discovery::instance::identity};
                     auto reply = communication::ipc::call( device, common::message::counter::Request( process::handle()));
                     create_formatter().print( std::cout, metric::transform( reply));
                  };

                  return argument::Option{
                     std::move( invoke),
                     { "--metric"},
                     R"(list metrics

List counts of _discovery tasks_ the domain-discovery has in-flight and completed

* discovery-out: 
   Requests to other domains
* discovery-in: 
   Requests from other domains
* lookup: 
   Requests to local providers that has service/queues
* fetch-known:
   Requests to local providers about the total set of known service/queues

@attention INCUBATION - might change during, or in between minor version.
)"
                  };
               }

               namespace message::count
               {
                  auto option()
                  {
                     auto invoke = []()
                     {
                        log::line( std::cerr, "@removed use `casual internal --message-count <pid>` instead");
                     };

                     return argument::Option{
                        std::move( invoke),
                        argument::option::keys( {}, { "--metric-message-count"}),
                        R"(@removed use `casual internal --message-count <pid>` instead)"
                     };
                  }
               } // message::count
            } // metric

         } // <unnamed>
      } // local

      struct cli::Implementation
      {
         argument::Group options()
         {
            return { [](){}, { "discovery"}, "responsible for discovery stuff",
               local::list::providers::option(),
               local::discover::services::option(),
               local::discover::queues::option(),
               local::rediscovery::option(),
               local::metric::option(),
               local::metric::message::count::option(),
               casual::cli::state::option( &local::call::state)
            };
         }

      };

      cli::cli() = default;
      cli::~cli() = default;

      argument::Group cli::options() &
      {
         return m_implementation->options();
      }

   } // domain::discovery::admin
} // casual
