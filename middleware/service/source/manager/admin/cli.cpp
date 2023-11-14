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
                     auto reply = call( casual::domain::manager::admin::service::name::state);

                     reply >> CASUAL_NAMED_VALUE( state.domain);

                     state.service = admin::api::state();

                     return state;
                  }

               } // call

               namespace normalized
               {
                  struct Instance :  admin::model::instance::Base
                  {
                     enum class State : char
                     {
                        idle,
                        busy,
                        remote,
                     };

                     Instance( const admin::model::instance::Sequential& sequential) : admin::model::instance::Base{ sequential}, state{ static_cast< State>( sequential.state)} {}
                     Instance( const admin::model::instance::Concurrent& concurrent) : admin::model::instance::Base{ concurrent}, state{ State::remote} {}

                     State state;
                  };

                  std::vector< Instance> instances( const admin::model::State& state)
                  {
                     std::vector< Instance> result;

                     algorithm::container::append( state.instances.sequential, result);
                     algorithm::container::append( state.instances.concurrent, result);

                     return result;
                  }

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
                  struct state_base : std::reference_wrapper< const admin::model::State>
                  {
                     using std::reference_wrapper< const admin::model::State>::reference_wrapper;
                  };

                  namespace instance
                  {
                     struct base_instances
                     {
                        using instances_type = std::vector< normalized::Instance>;
                        using range_type = range::type_t< instances_type>;

                        base_instances( instances_type& instances) : m_instances( instances) {}

                        template< typename T>
                        range_type instances( const std::vector< T>& instances) const
                        {
                           // We want to find the intersection between the argument instances and the
                           // total instances we hold
                           return std::get< 0>( algorithm::intersection( m_instances, instances, []( const normalized::Instance& ni, const T& i){
                              return ni.process.pid == i.pid;
                           }));
                        }

                        instances_type& m_instances;
                     };


                     namespace local
                     {
                        struct total
                        {
                           std::size_t operator () ( const admin::model::Service& value) const
                           {
                              return value.instances.sequential.size();
                           }
                        };


                        struct busy : base_instances
                        {
                           using base_instances::base_instances;

                           std::size_t operator () ( const admin::model::Service& value) const
                           {
                              auto instances = base_instances::instances( value.instances.sequential);

                              return algorithm::count_if( instances, []( const normalized::Instance& i){
                                 return i.state == normalized::Instance::State::busy;
                              });
                           }
                        };

                        struct pending_base : state_base
                        {
                           using state_base::state_base;

                           std::size_t pending( const std::string& service) const
                           {
                              return algorithm::count_if( get().pending, [&]( const admin::model::Pending& p){
                                 return p.requested == service;
                              });
                           }
                        };

                        struct pending : pending_base
                        {
                           using pending_base::pending_base;

                           std::size_t operator () ( const admin::model::Service& value) const
                           {
                              return pending_base::pending( value.name);
                           }
                        };

                     } // local

                     namespace remote
                     {
                        struct total
                        {
                           std::size_t operator () ( const admin::model::Service& value) const
                           {
                              return value.instances.concurrent.size();
                           }
                        };

                     } // remote
                  } // instances

                  struct format_instances
                  {
                     template< typename T>
                     std::size_t operator () ( const T& value) const { return value.instances.size();}
                  };

                  auto services( const admin::model::State& state)
                  {

                     static auto instances = normalized::instances( state);

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
                           terminal::format::column( "I", format::instance::local::total{}, terminal::color::white, terminal::format::Align::right),
                           terminal::format::column( "C", format_invoked, terminal::color::white, terminal::format::Align::right),
                           terminal::format::column( "AT", format_avg_time, terminal::color::white, terminal::format::Align::right),
                           terminal::format::column( "min", format_min_time, terminal::color::white, terminal::format::Align::right),
                           terminal::format::column( "max", format_max_time, terminal::color::white, terminal::format::Align::right),
                           terminal::format::column( "P", format_pending_count, terminal::color::magenta, terminal::format::Align::right),
                           terminal::format::column( "PAT", format_avg_pending_time, terminal::color::magenta, terminal::format::Align::right),
                           terminal::format::column( "RI", format::instance::remote::total{}, terminal::color::cyan, terminal::format::Align::right),
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
                           terminal::format::column( "I", format::instance::local::total{}),
                           terminal::format::column( "C", format_invoked),
                           terminal::format::column( "AT", format_avg_time),
                           terminal::format::column( "min", format_min_time),
                           terminal::format::column( "max", format_max_time),
                           terminal::format::column( "P", format_pending_count),
                           terminal::format::column( "PAT", format_avg_pending_time),
                           terminal::format::column( "RI", format::instance::remote::total{}),
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
                  enum class Service
                  {
                     user,
                     admin
                  };

                  void services( std::ostream& out, admin::model::State& state, Service type)
                  {
                     auto split = algorithm::partition( state.services, []( const admin::model::Service& s)
                     {
                        return ! algorithm::compare::any( s.category, common::service::category::admin, common::service::category::deprecated);
                     });

                     auto services = type == Service::user ? std::get< 0>( split) : std::get< 1>( split);

                     algorithm::sort( services, []( const auto& l, const auto& r){ return l.name < r.name;});

                     auto formatter = format::services( state);

                     formatter.print( out, services);
                  }

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

               namespace action
               {

                  auto services_completer = []( auto value, bool help)
                  {
                     if( help)
                     {
                        return std::vector< std::string>{ "<service name>..."};
                     }
                     else
                     {
                        auto state = admin::api::state();

                        return common::algorithm::transform( state.services, []( auto& service){
                           return std::move( service.name);
                        });
                     }
                  };

                  namespace list
                  {
                     namespace services
                     {
                        auto invoke()
                        {
                           auto state = admin::api::state();

                           print::services( std::cout, state, print::Service::user);
                        }

                        constexpr auto description = R"()";

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

                     } // service
                  } // list


                  void list_admin_services()
                  {
                     auto state = admin::api::state();

                     print::services( std::cout, state, print::Service::admin);
                  }

                  void list_instances()
                  {
                     auto state = call::instances();

                     print::instances( std::cout, state);
                  }

                  namespace metric
                  {

                     void reset( const std::vector< std::string>& services)
                     {
                        call::metric::reset( services);
                     }

                  } // metric

                  namespace routes
                  {
                     void list() 
                     {
                        auto state = admin::api::state();

                        auto formatter = format::routes( state);
                        formatter.print( std::cout, state.routes);
                     }
                     constexpr auto description = "list service routes";
                  } // routes

                  namespace legend
                  {
                     const std::map< std::string, const char*> legends{
                        { "list-services", action::list::services::legend},
                        { "list-admin-services", action::list::services::legend},
                     };
               
                     void invoke( const std::string& option)
                     {
                        auto found = algorithm::find( legend::legends, option);

                        if( found)
                           std::cout << found->second;
                        else if( option == "list-service") // Only for backward compatibility
                           std::cout << action::list::services::legend;
                        else
                           code::raise::error( code::casual::invalid_argument, "not a valid argument to --legend: ", option);
                     }

                     auto complete() 
                     {
                        return []( auto values, auto help)
                        {
                           return algorithm::transform( legends, []( auto& pair){ return pair.first;});
                        };
                     }


                     constexpr auto description = R"(the legend for the supplied option

Documentation and description for abbreviations and acronyms used as columns in output

note: not all options has legend, use 'auto complete' to find out which legends are supported.
      )";

                  } // legend

                  namespace information
                  {
                     auto call() -> std::vector< std::tuple< std::string, std::string>>
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
                     }

                     void invoke()
                     {
                        terminal::formatter::key::value().print( std::cout, call());
                     }

                     constexpr auto description = R"(collect aggregated information about services in this domain)";
                  } // information   

               } // action
            } // <unnamed>
         } // local

         namespace admin 
         {
            struct cli::Implementation
            {
               common::argument::Group options()
               {
                  return common::argument::Group{ [](){}, { "service"}, "service related administration",
                     common::argument::Option{ &local::action::list::services::invoke , { "-ls", "--list-services"}, local::action::list::services::description},
                     common::argument::Option{ &local::action::list_instances,  { "-li", "--list-instances"}, "list instances"},
                     common::argument::Option{ &local::action::routes::list, { "--list-routes"}, local::action::routes::description},
                     common::argument::Option{ &local::action::metric::reset, local::action::services_completer,  { "-mr", "--metric-reset"}, "reset metrics for provided services, if no services provided, all metrics will be reset"},
                     common::argument::Option{ &local::action::list_admin_services,  { "--list-admin-services"}, "list casual administration services"},
                     common::argument::Option{ &local::action::legend::invoke, local::action::legend::complete(), { "--legend"}, local::action::legend::description},
                     common::argument::Option{ &local::action::information::invoke, { "--information"}, local::action::information::description},
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
               return local::action::information::call();
            }
            
         } // admin 

      } // manager
   } // service
} // casual


