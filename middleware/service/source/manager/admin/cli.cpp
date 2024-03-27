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

#include "common/argument.h"
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

   namespace service
   {
      namespace manager
      {
         namespace local
         {
            namespace
            {
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


                  struct State
                  {
                     admin::model::State service;
                     casual::domain::manager::admin::model::State domain;
                  };

                  State instances()
                  {
                     State state;

                     serviceframework::service::protocol::binary::Call call;
                     state.domain = call( casual::domain::manager::admin::service::name::state).extract< casual::domain::manager::admin::model::State>();
                     state.service = admin::api::state();

                     return state;
                  }

               } // call

               namespace normalized
               {
                  namespace service
                  {

                     struct Instance
                     {
                        enum class State : short
                        {
                           idle,
                           busy,
                           remote,
                           exiting,
                        };

                        Instance( const admin::model::Service& service) : service{ service} {}

                        common::process::Handle process;
                        std::size_t hops = 0;
                        std::string description;

                        std::reference_wrapper< const admin::model::Service> service;
                        const casual::domain::manager::admin::model::Server* server = nullptr;

                        State state = State::remote;
                     };

                     namespace local
                     {
                        namespace
                        {
                           namespace lookup
                           {
                              const casual::domain::manager::admin::model::Server* server( const call::State& state, strong::process::id pid)
                              {
                                 auto found = algorithm::find_if( state.domain.servers, [pid]( auto& e){
                                    return algorithm::find( e.instances, pid);
                                 });

                                 if( found)
                                    return &(*found);

                                 return nullptr;
                              }

                           } // lookup
                        } // <unnamed>
                     } // local

                     std::vector< Instance> instances( const call::State& state)
                     {
                        std::vector< Instance> result;

                        for( auto& service : state.service.services)
                        {
                           for( auto& i : service.instances.sequential)
                           {
                              auto local = algorithm::find( state.service.instances.sequential, i.pid);
                              Instance instance{ service};
                              instance.state = local->state == admin::model::instance::Sequential::State::idle ? Instance::State::idle : Instance::State::busy;
                              instance.process.pid = i.pid;
                              instance.server = local::lookup::server( state, i.pid);
                              result.push_back( std::move( instance));
                           }

                           for( auto& i : service.instances.concurrent)
                           {
                              Instance instance{ service};
                              instance.process.pid = i.pid;
                              instance.hops = i.hops;
                              instance.server = local::lookup::server( state, i.pid);
                              result.push_back( std::move( instance));
                           }
                        }

                        return result;
                     }

                  } // service

               } // normalized


               namespace format
               {
                  auto services()
                  {

                     auto format_timeout_duration_string = []( const auto& value) -> std::string
                     {
                        if( ! value.execution.timeout.duration || *value.execution.timeout.duration == platform::time::unit::zero())
                           return "-";
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
                           return "-";
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
                           return "-";
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
                           return "-";

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

                  auto routes( const admin::model::State& state)
                  {
                     return terminal::format::formatter< admin::model::Route>::construct( 
                        terminal::format::column( "name", std::mem_fn( &admin::model::Route::service), terminal::color::yellow, terminal::format::Align::left),
                        terminal::format::column( "target", std::mem_fn( &admin::model::Route::target), terminal::color::no_color, terminal::format::Align::left)
                     );
                  }


                  auto instances()
                  {
                     using value_type = normalized::service::Instance;

                     auto format_pid = []( auto& v){ return v.process.pid;};

                     auto format_service_name = []( const value_type& v)
                     {
                        return v.service.get().name;
                     };

                     auto format_process_alias = []( const value_type& v) -> const std::string&
                     {
                        if( v.server) 
                           return v.server->alias;

                        static std::string empty;
                        return empty;
                     };

                     struct format_state
                     {
                        std::size_t width( const value_type& value, const std::ostream&) const
                        {
                           using Enum = value_type::State;
                           switch( value.state)
                           {
                              case Enum::idle: return 4;
                              case Enum::busy: return 4;
                              case Enum::remote: return 6;
                              case Enum::exiting: return 7;
                           }
                           return 0;
                        }

                        void print( std::ostream& out, const value_type& value, std::size_t width) const
                        {
                           out << std::setfill( ' ');

                           using Enum = value_type::State;
                           switch( value.state)
                           {
                              case Enum::idle: out << std::left << std::setw( width) << terminal::color::green << "idle"; break;
                              case Enum::busy: out << std::left << std::setw( width) << terminal::color::yellow << "busy"; break;
                              case Enum::remote: out << std::left << std::setw( width) << terminal::color::cyan << "remote"; break;
                              case Enum::exiting: out << std::left << std::setw( width) << terminal::color::magenta << "exiting"; break;
                           }
                        }
                     };

                     auto format_hops = []( const value_type& value)
                     {
                        return value.hops;
                     };

                     return terminal::format::formatter< normalized::service::Instance>::construct(
                        terminal::format::column( "service", format_service_name, terminal::color::yellow),
                        terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
                        terminal::format::custom::column( "state", format_state{}),
                        terminal::format::column( "hops", format_hops, terminal::color::no_color, terminal::format::Align::right),
                        terminal::format::column( "alias", format_process_alias, terminal::color::blue, terminal::format::Align::left)
                     );
                  }

               } // format


               namespace print
               {

                  template< typename IR>
                  void instances( std::ostream& out, IR instances)
                  {
                     auto formatter = format::instances();

                     formatter.print( out, instances);
                  }

                  void instances( std::ostream& out, call::State& state)
                  {
                     auto instances = normalized::service::instances( state);

                     auto sort_predicate = []( auto& l, auto& r)
                     {
                        auto tie = []( auto& value){ return std::tie( value.service.get().name, value.server);};

                        return tie( l) < tie( r);
                     };

                     algorithm::stable_sort( instances, sort_predicate);

                     print::instances( out, instances);
                  }

               } // print

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
                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto is_not_admin = []( auto& service)
                           { 
                              return ! algorithm::compare::any( service.category, common::service::category::admin, common::service::category::deprecated);
                           };

                           auto state = manager::admin::api::state();
                           auto services = algorithm::sort( algorithm::filter( state.services, is_not_admin));

                           format::services().print( std::cout, services);
                        };

                        return common::argument::Option{ 
                           invoke,
                           { "-ls", "--list-services"}, 
                           R"(list known services)"};
                     }

                     namespace admin
                     {
                        auto option()
                        {
                           auto invoke = []()
                           {
                              auto is_admin = []( auto& service)
                              { 
                                 return algorithm::compare::any( service.category, common::service::category::admin, common::service::category::deprecated);
                              };

                              auto state = manager::admin::api::state();
                              auto services = algorithm::sort( algorithm::filter( state.services, is_admin));

                              format::services().print( std::cout, services);
                           };

                           return common::argument::Option{ 
                              invoke,
                              { "--list-admin-services"}, 
                              "list casual administration services"};
                        }

                     } // admin

                   } // services

                   namespace instances
                   {
                     auto option()
                     {
                        auto invoke = []()
                        {
                           auto state = call::instances();

                           print::instances( std::cout, state);
                        };
                        
                        return common::argument::Option{ 
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

                           auto formatter = format::routes( state);
                           formatter.print( std::cout, state.routes);
                        };

                        return common::argument::Option{ 
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

                     auto completer = []( auto value, bool help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<service>..."};
                        
                        auto state = admin::api::state();

                        return common::algorithm::transform( state.services, []( auto& service){
                           return std::move( service.name);
                        });
                     };
                     

                     return common::argument::Option{ 
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
                     static const std::map< std::string, std::string_view> legends{
                        { "list-services", list::services::legend},
                        { "list-admin-services", list::services::legend},
                     };

                     auto invoke = []( const std::string& value)
                     {
                        if( auto found = algorithm::find( legends, value))
                           std::cout << found->second;
                        else
                           code::raise::error( code::casual::invalid_argument, "not a valid argument to --legend: ", value);
                     };

                     auto completer = []( auto values, auto help) -> std::vector< std::string>
                     {
                        if( help)
                           return { "<option>"};

                        return algorithm::transform( legends, []( auto& pair){ return pair.first;});
                     };
                     
                     return common::argument::Option{ 
                        invoke,
                        completer,
                        { "--legend"}, 
                         R"(the legend for the supplied option

Documentation and description for abbreviations and acronyms used as columns in output

note: not all options has legend, use 'auto complete' to find out which legends are supported.
)"};
                  }
                  
               } // legend

               namespace information
               {
                  auto compose() -> std::vector< std::tuple< std::string, std::string>>
                  {
                     auto state = admin::api::state();

                     auto category_predicate = []( auto category){ return [=]( auto& service){ return service.category == category;};};

                     auto split = algorithm::partition( state.services, category_predicate( ".admin"));
                     auto admin = std::get< 0>( split);
                     auto services = std::get< 1>( split);

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
                        { "service.manager.service.admin.count", string::compose( admin.size())},
                        { "service.manager.service.admin.metric.invoked.count", string::compose( invoked_count( admin))},
                        { "service.manager.service.count", string::compose( services.size())},
                        { "service.manager.service.metric.invoked.count", string::compose( invoked_count( services))},
                        { "service.manager.service.metric.invoked.total", string::compose( invoked_total( services))},
                        { "service.manager.service.metric.invoked.average", string::compose( average( invoked_total( services), invoked_count( services)))},
                        { "service.manager.service.metric.remote", string::compose( remote_count( services))},
                        { "service.manager.service.metric.pending.count", string::compose( pending_count( services))},
                        { "service.manager.service.metric.pending.total", string::compose( pending_total( services))},
                        { "service.manager.service.metric.pending.average", string::compose( average( pending_total( services), pending_count( services)))},
                     };
                  };

                  auto option()
                  {
                     auto invoke = []()
                     {
                        terminal::formatter::key::value().print( std::cout, information::compose());
                     };

                     return common::argument::Option{ 
                        invoke,
                        { "--information"}, 
                        R"(collect aggregated information about known services)"};

                  }
               } // information

            } // <unnamed>
         } // local

         namespace admin 
         {
            struct cli::Implementation
            {
               common::argument::Group options()
               {
                  return common::argument::Group{ [](){}, { "service"}, "service related administration",
                     local::list::services::option(),
                     local::list::instances::option(),
                     local::list::routes::option(),
                     local::metric::reset::option(),
                     local::list::services::admin::option(),
                     local::legend::option(),
                     local::information::option(),
                     casual::cli::state::option( &api::state),
                  };
               }
            };

            cli::cli() = default; 
            cli::~cli() = default; 

            common::argument::Group cli::options() &
            {
               return m_implementation->options();
            }

            std::vector< std::tuple< std::string, std::string>> cli::information() &
            {
               return local::information::compose();
            }
            
         } // admin 

      } // manager
   } // service
} // casual


