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

#include "common/terminal.h"
#include "common/communication/ipc.h"
#include "common/serialize/create.h"
#include "common/stream.h"

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
                  platform::size::type inbound{};
                  platform::size::type outbound{};
                  
                  auto completed() const noexcept { return inbound;}
                  auto pending() const noexcept { return outbound - inbound;}
               };

               auto transform( const discovery::admin::model::State& state) noexcept
               {
                  auto create_row = [ &state]( std::string_view name, common::message::Type sent, common::message::Type received)
                  {
                     return metric::Row{
                        name,
                        algorithm::find( state.metric.message.count.receive, received)->value,
                        algorithm::find( state.metric.message.count.send, sent)->value
                     };

                  };

                  return std::vector< Row>{
                     create_row( "discovery", common::message::Type::domain_discovery_request, common::message::Type::domain_discovery_reply),
                     create_row( "lookup", common::message::Type::domain_discovery_lookup_request, common::message::Type::domain_discovery_lookup_reply),
                     create_row( "fetch-known", common::message::Type::domain_discovery_fetch_known_request, common::message::Type::domain_discovery_fetch_known_reply),
                  };
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
                     auto state = local::call::state();
                     create_formatter().print( std::cout, metric::transform( state));
                  };

                  return argument::Option{
                     std::move( invoke),
                     { "--metric"},
                     R"(list metrics

List counts of _discovery tasks_ the domain-discovery has (begun) executed.

* external-discovery: 
   Requests to other domains
* internal-discovery: 
   Requests to local providers that supplies _resources_ (service/queues)
* local-known-request:
   Requests to local providers about the total set of known _resources_ (service/queues)
* local-needs-request
   Requests to local providers about resources that are needed but not known

@attention INCUBATION - might change during, or in between minor version.
)"
                  };
               }

               namespace message::count
               {
                  struct Row : common::compare::Order< Row>
                  {
                     Row( common::message::Type type, std::string_view bound, platform::size::type count)
                        : type{ type}, bound{ bound}, count{ count} {}

                     common::message::Type type;
                     std::string_view bound;
                     platform::size::type count{};

                     inline auto tie() const noexcept { return std::tie( type, bound);}
                  };

                  auto transform( const admin::model::State& state)
                  {
                     auto transform_value = []( std::string_view bound)
                     {
                        return [ bound]( auto& count)
                        {
                           return count::Row{ count.type, bound, count.value};
                        };
                     };
                     auto result = algorithm::transform( state.metric.message.count.send, transform_value( "sent"));
                     algorithm::transform( state.metric.message.count.receive, std::back_inserter( result), transform_value( "received"));

                     return algorithm::sort( std::move( result));
                  }

                  auto option()
                  {
                     static constexpr auto create_formatter = []()
                     {
                        return terminal::format::formatter< Row>::construct(
                           terminal::format::column( "type", std::mem_fn( &Row::type), terminal::color::yellow, terminal::format::Align::left),
                           terminal::format::column( "count", std::mem_fn( &Row::count), terminal::color::cyan, terminal::format::Align::right),
                           terminal::format::column( "bound", std::mem_fn( &Row::bound), terminal::color::cyan, terminal::format::Align::left)
                        );
                     };

                     auto invoke = []()
                     {
                        auto state = local::call::state();
                        create_formatter().print( std::cout, count::transform( state));
                     };

                     return argument::Option{
                        std::move( invoke),
                        { "--metric-message-count"},
                        R"(list message counts

Lists counts of "all" internal messages that has been sent and received. 

@attention These might change over time, and between minor versions.
)"
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
