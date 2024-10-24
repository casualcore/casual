//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "service/manager/admin/cli.h"

#include "service/manager/admin/model.h"
#include "service/manager/admin/server.h"
#include "service/manager/admin/api.h"

#include "domain/manager/admin/model.h"
#include "domain/manager/admin/server.h"

#include "casual/argument.h"
#include "common/chronology.h"
#include "common/terminal.h"
#include "common/server/service.h"
#include "common/exception/capture.h"
#include "common/algorithm/compare.h"
#include "common/serialize/macro.h"
#include "common/serialize/create.h"
#include "common/algorithm/container.h"

#include "serviceframework/service/protocol/call.h"

#include "casual/cli/state.h"

#include <iostream>
#include <iomanip>
#include <limits>

namespace casual
{

   using namespace common;

   namespace service::manager::admin::cli
   {
         namespace local
         {
            namespace
            {
               struct State
               {
                  bool all = false;
               };

               namespace call
               {
                  namespace metric
                  {

                     std::vector< std::string> reset( const std::vector< std::string>& services)
                     {
                        auto reply = serviceframework::service::protocol::binary::Call{}( admin::service::name::metric::reset, services);
                        return reply.extract< std::vector< std::string>>();
                     }

                  } // metric

               } // call

               namespace normalized
               {
                  namespace instance
                  {
                     enum class State : short
                     {
                        idle,
                        busy,
                        external,
                     };

                     constexpr std::string_view description( State value) noexcept
                     {
                        switch( value)
                        {
                           case State::idle: return "idle";
                           case State::busy: return "busy";
                           case State::external: return "external";
                        }
                        return "<unknown>";
                     }

                  } // instance

                  struct Instance : compare::Order< Instance>
                  {
                     std::string service;
                     instance::State state{};
                     common::process::Handle process;
                     std::string alias;
                     std::string description;
                     platform::size::type hops{};

                     auto tie() const noexcept { return std::tie( service, hops, alias, description);}                     
                  };

                  std::vector< Instance> instances( const admin::model::State& state)
                  {
                     // maybe_unused to silence g++ bug
                     [[maybe_unused]] static auto transform_sequential = []( const admin::model::State& state, const std::string& service)
                     {
                        return [ &state, &service]( const admin::model::service::instance::Sequential& instance)
                        {
                           if( auto found = algorithm::find( state.instances.sequential, instance.process))
                           {
                              Instance result;
                              result.service = service;
                              result.alias = found->alias;
                              result.process = instance.process;
                              result.state = found->state == decltype( found->state)::idle ? instance::State::idle : instance::State::busy;
                              return result;
                           }
                           code::raise::error( code::casual::internal_unexpected_value, "failed to find instance: ", instance.process);
                        };
                     };

                     // maybe_unused to silence g++ bug
                     [[maybe_unused]] static auto transform_concurrent = []( const admin::model::State& state, const std::string& service)
                     {
                        return [ &state, &service]( const admin::model::service::instance::Concurrent& instance)
                        {
                           if( auto found = algorithm::find( state.instances.concurrent, instance.process))
                           {
                              Instance result;
                              result.service = service;
                              result.alias = found->alias;
                              result.description = found->description;
                              result.process = instance.process;
                              result.state = instance::State::external;
                              return result;
                           }
                           code::raise::error( code::casual::internal_unexpected_value, "failed to find instance: ", instance.process);
                        };
                     };

                     return algorithm::accumulate( state.services, std::vector< Instance>{}, [ &state]( auto result, auto& service)
                     {
                        algorithm::transform( service.instances.sequential, result, transform_sequential( state, service.name));
                        algorithm::transform( service.instances.concurrent, result, transform_concurrent( state, service.name));
                        return result;
                     });
                  }                  

               } // normalized

               namespace format
               {
                  auto empty_representation()
                  {
                     if( ! terminal::output::directive().porcelain())
                        return "-";

                     return "";
                  }
                  
                  // this kind of stuff should maybe be in the terminal abstraction?
                  auto hyphen_if_empty( std::string_view value) -> std::string_view 
                  {
                     if( value.empty())
                        return empty_representation();
                     return value;
                  }

                  auto services()
                  {

                     auto format_timeout_duration_string = []( const auto& value) -> std::string
                     {
                        if( ! value.execution.timeout.duration || *value.execution.timeout.duration == platform::time::unit::zero())
                           return empty_representation(); 
                        using second_t = std::chrono::duration< double>;
                        return std::to_string( std::chrono::duration_cast< second_t>( value.execution.timeout.duration.value()).count());
                     };

                     // porcelain
                     auto format_timeout_duration_double = []( const auto& value)
                     {
                        if( ! value.execution.timeout.duration)
                           return 0.0;
                        using second_t = std::chrono::duration< double>;
                        return std::chrono::duration_cast< second_t>( value.execution.timeout.duration.value()).count();
                     };

                     auto format_timeout_contract = []( const auto& value) -> std::string_view
                     {
                         if( ! value.execution.timeout.contract)
                           return empty_representation();
                        return description( *value.execution.timeout.contract);
                     };

                     auto format_sequential_instances = []( const admin::model::Service& value)
                     {
                        return value.instances.sequential.size();
                     };

                     auto format_concurrent_instances = []( const admin::model::Service& value)
                     {
                        return value.instances.concurrent.size();
                     };

                     // we need to set something when category is empty to help
                     // enable possible use of sort, cut, awk and such
                     auto format_category = []( const admin::model::Service& value) -> std::string_view
                     {
                        if( value.category.empty())
                           return empty_representation();
                        return value.category;
                     };

                     auto format_visibility = []( auto& value) -> std::string_view
                     {
                        using Enum = decltype( value.visibility);
                        switch( value.visibility)
                        {
                           case Enum::discoverable: return "D";
                           case Enum::undiscoverable: return "U";
                        }
                        return "<unknown>";
                     };


                     auto format_invoked = []( const admin::model::Service& value)
                     {
                        return value.metric.invoked.count;
                     };

                     using time_type = std::chrono::duration< double>;

                     auto format_avg_time = []( const admin::model::Service& value)
                     {
                        if( value.metric.invoked.count == 0)
                           return 0.0;

                        return std::chrono::duration_cast< time_type>( value.metric.invoked.total / value.metric.invoked.count).count();
                     };

                     auto format_min_time = []( const admin::model::Service& value)
                     {
                        return std::chrono::duration_cast< time_type>( value.metric.invoked.limit.min).count();
                     };

                     auto format_max_time = []( const admin::model::Service& value)
                     {
                        return std::chrono::duration_cast< time_type>( value.metric.invoked.limit.max).count();
                     };

                     auto format_pending_count = []( const admin::model::Service& value)
                     {
                        return value.metric.pending.count;
                     };

                     auto format_avg_pending_time = []( const admin::model::Service& value)
                     {
                        if( value.metric.pending.count == 0)
                           return 0.0;

                        return std::chrono::duration_cast< time_type>( value.metric.pending.total / value.metric.pending.count).count();
                     };

                     auto format_last = []( const admin::model::Service& value) -> std::string
                     {
                        if( value.metric.last == platform::time::point::limit::zero())
                           return empty_representation();

                        return common::chronology::utc::offset( value.metric.last);
                     };

                     auto remote_invocations = []( auto& value){ return value.metric.remote;};

                     if( ! terminal::output::directive().porcelain())
                     {
                        return terminal::format::formatter< admin::model::Service>::construct( 
                           terminal::format::column( "name", std::mem_fn( &admin::model::Service::name), terminal::color::yellow, terminal::format::Align::left),
                           terminal::format::column( "category", format_category, terminal::color::no_color, terminal::format::Align::left),
                           terminal::format::column( "V", format_visibility, terminal::color::no_color, terminal::format::Align::left),
                           terminal::format::column( "mode", std::mem_fn( &admin::model::Service::transaction), terminal::color::no_color, terminal::format::Align::left),
                           terminal::format::column( "timeout", format_timeout_duration_string, terminal::color::blue, terminal::format::Align::right),
                           terminal::format::column( "contract", format_timeout_contract, terminal::color::blue, terminal::format::Align::right),
                           terminal::format::column( "I", format_sequential_instances, terminal::color::white, terminal::format::Align::right),
                           terminal::format::column( "C", format_invoked, terminal::color::white, terminal::format::Align::right),
                           terminal::format::column( "AT", format_avg_time, terminal::color::white, terminal::format::Align::right),
                           terminal::format::column( "min", format_min_time, terminal::color::white, terminal::format::Align::right),
                           terminal::format::column( "max", format_max_time, terminal::color::white, terminal::format::Align::right),
                           terminal::format::column( "P", format_pending_count, terminal::color::magenta, terminal::format::Align::right),
                           terminal::format::column( "PAT", format_avg_pending_time, terminal::color::magenta, terminal::format::Align::right),
                           terminal::format::column( "RI", format_concurrent_instances, terminal::color::cyan, terminal::format::Align::right),
                           terminal::format::column( "RC", remote_invocations, terminal::color::cyan, terminal::format::Align::right),
                           terminal::format::column( "last", format_last, terminal::color::blue, terminal::format::Align::left)
                        );
                     }
                     else
                     {
                        return terminal::format::formatter< admin::model::Service>::construct( 
                           terminal::format::column( "name", std::mem_fn( &admin::model::Service::name)),
                           terminal::format::column( "category", format_category),
                           terminal::format::column( "mode", std::mem_fn( &admin::model::Service::transaction)),
                           terminal::format::column( "timeout", format_timeout_duration_double),
                           terminal::format::column( "I", format_sequential_instances),
                           terminal::format::column( "C", format_invoked),
                           terminal::format::column( "AT", format_avg_time),
                           terminal::format::column( "min", format_min_time),
                           terminal::format::column( "max", format_max_time),
                           terminal::format::column( "P", format_pending_count),
                           terminal::format::column( "PAT", format_avg_pending_time),
                           terminal::format::column( "RI", format_concurrent_instances),
                           terminal::format::column( "RC", remote_invocations),
                           terminal::format::column( "last", format_last),
                           terminal::format::column( "contract", format_timeout_contract),
                           terminal::format::column( "V", format_visibility)
                        );
                     }
                  }


                  auto routes()
                  {
                     return terminal::format::formatter< model::Route>::construct( 
                        terminal::format::column( "name", std::mem_fn( &model::Route::service), terminal::color::yellow, terminal::format::Align::left),
                        terminal::format::column( "target", std::mem_fn( &model::Route::target), terminal::color::no_color, terminal::format::Align::left)
                     );
                  }


                  auto instances()
                  {
                     auto format_pid = []( auto& instance){ return instance.process.pid;};

                     auto format_service_name = []( auto& instance) -> std::string_view
                     {
                        return instance.service;
                     };

                     auto format_alias = []( auto& instance) ->  std::string_view
                     {
                        return hyphen_if_empty( instance.alias);
                     };

                     auto format_description = []( auto& instance) ->  std::string_view
                     {
                        return hyphen_if_empty( instance.description);
                     };

                     struct format_state
                     {
                        std::size_t width( const normalized::Instance& instance, const std::ostream&) const
                        {
                           return description( instance.state).size();
                        }

                        void print( std::ostream& out, const normalized::Instance& instance, std::size_t width) const
                        {
                           out << std::setfill( ' ');

                           using State = decltype( instance.state);
                           switch( instance.state)
                           {
                              case State::idle: out << std::left << std::setw( width) << terminal::color::green << "idle"; break;
                              case State::busy: out << std::left << std::setw( width) << terminal::color::yellow << "busy"; break;
                              case State::external: out << std::left << std::setw( width) << terminal::color::cyan << "external"; break;
                           }
                        }
                     };

                     auto format_hops = []( auto& instance)
                     {
                        return instance.hops;
                     };

                     if( ! terminal::output::directive().porcelain())
                     {
                        return terminal::format::formatter< normalized::Instance>::construct(
                           terminal::format::column( "service", format_service_name, terminal::color::yellow),
                           terminal::format::custom::column( "state", format_state{}),
                           terminal::format::column( "hops", format_hops, terminal::color::no_color, terminal::format::Align::right),
                           terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
                           terminal::format::column( "alias", format_alias, terminal::color::blue, terminal::format::Align::left),
                           terminal::format::column( "description", format_description, terminal::color::yellow, terminal::format::Align::left)
                        );
                     }
                     else
                     {
                        return terminal::format::formatter< normalized::Instance>::construct(
                           terminal::format::column( "service", format_service_name),
                           terminal::format::column( "pid", format_pid),
                           terminal::format::custom::column( "state", format_state{}),
                           terminal::format::column( "hops", format_hops),
                           terminal::format::column( "alias", format_alias),
                           terminal::format::column( "description", format_description)
                        );

                     }
                  }

               } // format


               namespace list
               {
                  namespace services
                  {
                     constexpr auto legend = R"(
   name:
      the name of the service
   category:
      arbitrary category to help understand the 'purpose' with the service
   V:
      visibility - can be one of the following:
        - D: discoverable -> the service is discoverable from other domains
        - U: undiscoverable -> the service is NOT discoverable from other domains
   mode: 
      transaction mode - can be one of the following (auto, join, none, atomic)
   timeout:
      the timeout for the service (in seconds)
   contract:
      what happens if a timeout occur. 
   I:
      Instances - number of local instances
   C:
      calls - number of calls to the service
   AT:
      average-time - the average time of the service (in seconds)
   min:
      minimum-time - the minimum time of the service (in seconds)
   max:
      maximum-time - the maximum time of the service (in seconds)
   P
      Pending - total number of pending request, over time.
   PAT
      Pending-Average-Time - the average time request has waited for a service to be available, over time (in seconds)
   RI:
      Remote-Instances - number of remote instances
   RC:
      Remote-Calls - number of calls to remote instances (a subset of C)
   last:
      the last time the service was requested    
)";

                     auto option( auto shared)
                     {
                        auto invoke = [ shared]()
                        {
                           auto filter = [ &shared]( auto& service)
                           {
                              if( shared->all)
                                 return true; 
                              return ! common::service::hidden::name( service.name);
                           };

                           auto state = manager::admin::api::state();
                           auto services = algorithm::sort( algorithm::filter( state.services, filter));

                           format::services().print( std::cout, services);
                        };

                        auto flag = argument::Option{ [ shared]()
                           {
                              shared->all = true;
                              return argument::option::invoke::preemptive{};
                           },  { "-a", "--all"}, "include hidden services"};


                        constexpr auto description = R"(list known services)";

                        return argument::Option{ 
                           invoke,
                           { "-ls", "--list-services"}, 
                           description}( { std::move( flag)});
                     }
                   } // services

                   namespace instances
                   {
                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto state = admin::api::state();
                           auto instances = normalized::instances( state);

                           algorithm::sort( instances);

                           auto formatter = format::instances();
                           formatter.print( std::cout, instances);
                        };
                        
                        return argument::Option{ 
                           invoke,
                           { "-li", "--list-instances"}, 
                           "list instances"};
                     }
                   } // instances

                   namespace routes
                   {
                      auto option()
                      {
                        auto invoke = []()
                        {
                           auto state = admin::api::state();

                           auto formatter = format::routes();
                           formatter.print( std::cout, state.routes);
                        };

                        return argument::Option{ 
                           invoke,
                           { "--list-routes"}, 
                           "list service routes"};
                     }

                   } // routes
               } // list

               namespace metric::reset
               {
                  auto option()
                  {
                     auto invoke = []( const std::vector< std::string>& services)
                     {
                        call::metric::reset( services);
                     };

                     auto completer = []( bool help, auto value) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<service>..."};
                        
                        auto state = admin::api::state();

                        return common::algorithm::transform( state.services, []( auto& service){
                           return std::move( service.name);
                        });
                     };
                     

                     return argument::Option{ 
                        invoke,
                        completer,
                        { "-mr", "--metric-reset"}, 
                        "reset metrics for provided services, if no services provided, all metrics will be reset"};
                  }
                  
               } // metric::reset

               namespace legend
               {
                  auto option()
                  {
                     auto legend_option = [](  std::string key, std::string_view legend)
                     {
                        return argument::Option{ [ key, legend]()
                           {
                              std::cout << legend;
                           },
                           { key},
                           string::compose( "list legend for ", key)
                        };
                     };

                     return argument::Option{ 
                        [](){},
                        { "--legend"}, 
                         R"(the legend for the supplied option

Documentation and description for abbreviations and acronyms used as columns in output

The following options has legend:
)"
                     }( { legend_option( "--list-services", list::services::legend)});
                  }
                  
               } // legend

               namespace information
               {
                  auto compose() -> std::vector< std::tuple< std::string, std::string>>
                  {
                     auto state = admin::api::state();

                     auto is_hidden = []( auto& service){ return common::service::hidden::name( service.name);};

                     auto [ hidden, services] = algorithm::partition( state.services, is_hidden);

                     auto accumulate = []( auto extract)
                     {
                        return [extract]( auto& services)
                        {
                           decltype( extract( range::front( services))) initial{};
                           return algorithm::accumulate( services, initial, [extract]( auto count, auto& service){ return count + extract( service);});
                        };
                     };

                     auto remote_count = [accumulate]( auto& services)
                     {
                        return accumulate( []( auto& service){ return service.metric.remote;})( services);
                     };

                     auto invoked_count = [accumulate]( auto& services)
                     {
                        return accumulate( []( auto& service){ return service.metric.invoked.count;})( services);
                     };

                     using second_t = std::chrono::duration< double>;

                     auto invoked_total = [accumulate]( auto& services) -> second_t
                     {
                        return accumulate( []( auto& service){ return service.metric.invoked.total;})( services);
                     };
                     
                     auto pending_count = [accumulate]( auto& services)
                     {
                        return accumulate( []( auto& service){ return service.metric.pending.count;})( services);
                     };

                     auto pending_total = [accumulate]( auto& services) -> second_t
                     {
                        return accumulate( []( auto& service){ return service.metric.pending.total;})( services);
                     };

                     auto average = [=]( auto total, auto count) -> second_t
                     {
                        if( count == 0)
                           return {};

                        return std::chrono::duration_cast< second_t>( total / count);
                     };

                     return {
                        { "service.manager.service.route.count", string::compose( state.routes.size())},
                        { "service.manager.service.hidden.count", string::compose( hidden.size())},
                        { "service.manager.service.hidden.metric.invoked.count", string::compose( invoked_count( hidden))},
                        { "service.manager.service.count", string::compose( services.size())},
                        { "service.manager.service.metric.invoked.count", string::compose( invoked_count( services))},
                        { "service.manager.service.metric.invoked.total", string::compose( invoked_total( services))},
                        { "service.manager.service.metric.invoked.average", string::compose( average( invoked_total( services), invoked_count( services)))},
                        { "service.manager.service.metric.remote", string::compose( remote_count( services))},
                        { "service.manager.service.metric.pending.count", string::compose( pending_count( services))},
                        { "service.manager.service.metric.pending.total", string::compose( pending_total( services))},
                        { "service.manager.service.metric.pending.average", string::compose( average( pending_total( services), pending_count( services)))},
                        // deprecated
                        { "service.manager.service.admin.count", string::compose( hidden.size())},
                        { "service.manager.service.admin.metric.invoked.count", string::compose( invoked_count( hidden))},
                     };
                  };

                  auto option()
                  {
                     auto invoke = []()
                     {
                        terminal::formatter::key::value().print( std::cout, information::compose());
                     };

                     return argument::Option{ 
                        invoke,
                        { "--information"}, 
                        R"(collect aggregated information about known services)"};

                  }
               } // information

               namespace deprecated
               {
                  namespace admin_services
                  {
                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto is_hidden = []( auto& service)
                           { 
                              return common::service::hidden::name( service.name);
                           };

                           auto state = manager::admin::api::state();
                           auto services = algorithm::sort( algorithm::filter( state.services, is_hidden));

                           format::services().print( std::cout, services);
                        };

                        return argument::Option{ 
                           invoke,
                           argument::option::Names( {},{ "--list-admin-services"}), 
                           "@deprecated use --list-services --all"};
                     }

                  } // admin_services
               } // deprecated

            } // <unnamed>
         } // local

         std::vector< std::tuple< std::string, std::string>> information()
         {
            return local::information::compose();
         }

         argument::Option options()
         {  
            auto shared = std::make_shared< local::State>();        

            return argument::Option{ [](){}, { "service"}, "service related administration"}( {
               local::list::services::option( shared),
               local::list::instances::option(),
               local::list::routes::option(),
               local::metric::reset::option(),
               local::legend::option(),
               local::information::option(),
               casual::cli::state::option( &api::state),
               local::deprecated::admin_services::option(),
            });
         }

            
   } // service::manager::admin::cli
} // casual


