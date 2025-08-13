//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/manager/admin/cli.h"
#include "transaction/manager/admin/model.h"
#include "transaction/manager/admin/service/name.h"

#include "service/protocol/call.h"

#include "casual/argument.h"
#include "common/environment.h"
#include "common/terminal.h"
#include "common/exception/capture.h"
#include "common/serialize/create.h"
#include "common/communication/instance.h"
#include "common/communication/ipc.h"
#include "common/message/dispatch/handle.h"
#include "common/message/transaction.h"
#include "common/range/adapter.h"
#include "common/algorithm/sorted.h"


#include "casual/cli/pipe.h"
#include "casual/cli/state.h"


namespace casual
{
   using namespace common;

   namespace transaction::manager::admin::cli
   {

      namespace local
      {
         namespace
         {
            namespace call
            {
               auto state()
               {
                  casual::service::protocol::binary::Call call;
                  return call( service::name::state).extract< admin::model::State>();
               }

               namespace scale::resource::proxy
               {
                  auto instances( const std::vector< admin::model::scale::resource::proxy::Instances>& instances)
                  {
                     casual::service::protocol::binary::Call call;
                     return call( service::name::scale::resource::proxies, instances).extract< std::vector< admin::model::resource::Proxy>>();
                  }
               } // update

            } // call

            namespace normalized
            {
               auto compute_branch_count( const admin::model::State& state)
               {
                  std::map< common::strong::resource::id, platform::size::type> count;

                  for( auto& transaction : state.transactions)
                     for( auto& branch : transaction.branches)
                        for( auto& resource : branch.resources)
                           ++count[ resource.id];

                  return count;
               }

               namespace instance
               {
                  enum struct State : std::uint16_t
                  {
                     unknown,
                     spawned,
                     idle,
                     busy,
                     shutdown,
                     external,
                  };

                  constexpr std::string_view description( State value)
                  {
                     switch( value)
                     {
                        case State::unknown: return "unknown";
                        case State::spawned: return "spawned";
                        case State::idle: return "idle";
                        case State::busy: return "busy";
                        case State::shutdown: return "shutdown";
                        case State::external: return "external";
                     }
                     return "<unknown>";
                  }
                  
               } // instance

               struct Instance
               {
                  instance::State state{};
                  common::strong::resource::id id;
                  common::process::Handle process;
                  std::string alias;
                  std::string description;
               };

               //! fetch all instances, in a normalized form
               auto instances()
               {

                  auto accumulate_internal = []( auto result, auto& resource)
                  {
                     normalized::Instance value;
                     value.id = resource.id;
                     value.alias = resource.name;

                     return algorithm::accumulate( resource.instances, std::move( result), [ &value]( auto result, auto& instance)
                     {
                        static auto transform_state = []( auto state)
                        {
                           using From = decltype( state);
                           using To = normalized::instance::State;
                           switch( state)
                           {
                              case From::spawned: return To::spawned;
                              case From::idle: return To::idle;
                              case From::busy: return To::busy;
                              case From::shutdown: return To::shutdown;
                           }
                           return To::unknown;
                        };

                        value.process = instance.process;
                        value.state = transform_state( instance.state);
                        result.push_back( value);
                        return result;
                     });
                  };

                  auto transform_external = []( auto& instance)
                  {
                     normalized::Instance value;
                     value.state = decltype( value.state)::external;
                     value.id = instance.id;
                     value.process = instance.process;
                     value.alias = instance.alias;
                     value.description = instance.description;
                     return value;
                  };


                  auto state = call::state();

                  auto instances = algorithm::accumulate( state.resources, std::vector< normalized::Instance>{}, accumulate_internal);

                  algorithm::transform( state.externals, instances, transform_external);

                  return instances;
               }

            } // normalized


            namespace format
            {  
               using time_type = std::chrono::duration< double>;
               
               auto accumulate_statistics( const admin::model::resource::Proxy& value)
               {
                  struct Metrics
                  {
                     common::Metric resource;
                     common::Metric roundtrip;
                  };

                  struct Result
                  {
                     Metrics metrics;
                     common::Metric pending;
                  };

                  auto transform_metric = []( auto& v)
                  {  
                     common::Metric result;
                     result.count = v.count;
                     result.total = v.total;
                     result.limit.min = v.limit.min;
                     result.limit.max = v.limit.max;
                     return result;
                  };

                  Result result;
                  result.metrics.resource = transform_metric( value.metrics.resource);
                  result.metrics.roundtrip = transform_metric( value.metrics.roundtrip);
                  result.pending = transform_metric( value.pending);

                  for( auto& instance : value.instances)
                  {
                     result.metrics.resource += transform_metric( instance.metrics.resource);
                     result.metrics.roundtrip += transform_metric( instance.metrics.roundtrip);
                     result.pending += transform_metric( instance.pending);
                  }
                  return result;
               }

               auto transaction()
               {
                  auto format_global = []( auto& value) { return value.global.id;};
                  auto format_number_of_branches = []( auto& value) { return value.branches.size();};
                  auto format_owner = []( auto& value) -> std::string
                  {
                     if( value.owner)
                        return string::compose( value.owner);
                     else
                        return "-";
                  };

                  auto format_stage = []( auto& value)
                  {
                     return value.stage;
                  };

                  auto format_known = []( auto& value) { return terminal::format::guard_empty( common::chronology::utc::offset( value.known));};
                  auto format_deadline = []( auto& value) { return terminal::format::guard_empty( common::chronology::utc::offset( value.deadline));};

                  auto format_resources = []( auto& value)
                  {
                     auto resources = algorithm::accumulate( value.branches, std::vector< strong::resource::id>{}, []( auto result, auto& branch)
                     {
                        for( auto& resource : branch.resources)
                           algorithm::append_unique_value( resource.id, result);
                        return result;
                     });

                     return common::string::compose( resources);
                  };

                  if( ! terminal::output::directive().porcelain())
                  {
                     return common::terminal::format::formatter< admin::model::Transaction>::construct(
                        common::terminal::format::column( "global", format_global, common::terminal::color::yellow),
                        common::terminal::format::column( "#branches", format_number_of_branches, common::terminal::color::no_color),
                        common::terminal::format::column( "owner", format_owner, common::terminal::color::white, common::terminal::format::Align::right),
                        common::terminal::format::column( "stage", format_stage, common::terminal::color::green, common::terminal::format::Align::left),
                        common::terminal::format::column( "known", format_known, common::terminal::color::blue, common::terminal::format::Align::left),
                        common::terminal::format::column( "deadline", format_deadline, common::terminal::color::blue, common::terminal::format::Align::left),
                        common::terminal::format::column( "resources", format_resources, common::terminal::color::magenta, common::terminal::format::Align::left)
                     );
                  }
                  else
                  {
                     return common::terminal::format::formatter< admin::model::Transaction>::construct(
                        common::terminal::format::column( "global", format_global),
                        common::terminal::format::column( "#branches", format_number_of_branches),
                        common::terminal::format::column( "owner", format_owner),
                        common::terminal::format::column( "stage", format_stage),
                        common::terminal::format::column( "resources", format_resources),
                        common::terminal::format::column( "known", format_known),
                        common::terminal::format::column( "deadline", format_deadline)
                     );
                  }
               }

               constexpr std::string_view transactions_legend = R"(
global:
   global transaction id
#branches:
   number of branches in the transaction
owner:
   owner of the transaction, if any
stage:
   current stage of the transaction
known:
   the time point this TM knows about the transaction
deadline:
   the deadline of the transaction, if any
resources:
   the resources involved in the transaction, as a comma separated list of resource ids
)";

               auto resource_proxy( const std::map< common::strong::resource::id, platform::size::type>& branch_count)
               {

                  struct format_number_of_instances
                  {
                     std::size_t operator() ( const admin::model::resource::Proxy& value) const
                     {
                        return value.instances.size();
                     }
                  };

                  auto format_openinfo = []( const admin::model::resource::Proxy& value)
                  {
                     return terminal::format::guard_empty( value.openinfo);
                  };

                  auto format_closeinfo = []( const admin::model::resource::Proxy& value)
                  {
                     return terminal::format::guard_empty( value.closeinfo);
                  };

                  auto format_invoked = []( const admin::model::resource::Proxy& value)
                  {
                     auto result = accumulate_statistics( value);
                     return result.metrics.roundtrip.count;
                  };

                  auto format_branch_count = [ &branch_count]( const admin::model::resource::Proxy& value)
                  {
                     if( auto found = common::algorithm::find( branch_count, value.id))
                        return found->second;
                     return platform::size::type{ 0};
                  };

                  auto format_min = []( const admin::model::resource::Proxy& value)
                  {
                     auto result = accumulate_statistics( value);
                     return std::chrono::duration_cast< time_type>( result.metrics.roundtrip.limit.min).count();
                  };

                  auto format_max = []( const admin::model::resource::Proxy& value)
                  {
                     auto result = accumulate_statistics( value);
                     return std::chrono::duration_cast< time_type>( result.metrics.roundtrip.limit.max).count(); 
                  };

                  auto format_avg = []( const admin::model::resource::Proxy& value)
                  {
                     auto result = accumulate_statistics( value);
                     if( result.metrics.roundtrip.count == 0) 
                        return 0.0;
                     return std::chrono::duration_cast< time_type>( result.metrics.roundtrip.total / result.metrics.roundtrip.count).count();
                  };

                  auto format_pending_count = []( const admin::model::resource::Proxy& value)
                  {
                     return accumulate_statistics( value).pending.count;
                  };

                  auto format_avg_pending_time = []( const admin::model::resource::Proxy& value)
                  {
                     auto pending = accumulate_statistics( value).pending;
                     if( pending.count == 0) 
                        return 0.0;
                     return std::chrono::duration_cast< time_type>( pending.total / pending.count).count();
                  };

                  if( ! terminal::output::directive().porcelain())
                  {
                     return common::terminal::format::formatter< admin::model::resource::Proxy>::construct(
                        common::terminal::format::column( "name", std::mem_fn( &admin::model::resource::Proxy::name), common::terminal::color::yellow),
                        common::terminal::format::column( "id", std::mem_fn( &admin::model::resource::Proxy::id), common::terminal::color::yellow, terminal::format::Align::right),
                        common::terminal::format::column( "key", std::mem_fn( &admin::model::resource::Proxy::key), common::terminal::color::yellow),
                        common::terminal::format::column( "openinfo", format_openinfo, common::terminal::color::no_color),
                        common::terminal::format::column( "closeinfo", format_closeinfo, common::terminal::color::no_color),
                        common::terminal::format::column( "#B", format_branch_count, common::terminal::color::magenta, terminal::format::Align::right),
                        terminal::format::column( "invoked", format_invoked, terminal::color::blue, terminal::format::Align::right),
                        terminal::format::column( "min", format_min, terminal::color::blue, terminal::format::Align::right),
                        terminal::format::column( "max", format_max, terminal::color::blue, terminal::format::Align::right),
                        terminal::format::column( "avg", format_avg, terminal::color::blue, terminal::format::Align::right),
                        terminal::format::column( "P", format_pending_count, terminal::color::magenta, terminal::format::Align::right),
                        terminal::format::column( "PAT", format_avg_pending_time, terminal::color::magenta, terminal::format::Align::right),
                        terminal::format::column( "#", format_number_of_instances{}, terminal::color::white, terminal::format::Align::right)
                     );
                  }
                  else
                  {
                     // we need to keep compatibility with porcelain
                     return common::terminal::format::formatter< admin::model::resource::Proxy>::construct(
                        common::terminal::format::column( "name", std::mem_fn( &admin::model::resource::Proxy::name)),
                        common::terminal::format::column( "id", std::mem_fn( &admin::model::resource::Proxy::id)),
                        common::terminal::format::column( "key", std::mem_fn( &admin::model::resource::Proxy::key)),
                        common::terminal::format::column( "openinfo", format_openinfo),
                        common::terminal::format::column( "closeinfo", format_closeinfo),
                        terminal::format::column( "invoked", format_invoked),
                        terminal::format::column( "min", format_min),
                        terminal::format::column( "max", format_max),
                        terminal::format::column( "avg", format_avg),
                        terminal::format::column( "#", format_number_of_instances{}),
                        // last to keep compatibility
                        terminal::format::column( "P", format_pending_count),
                        terminal::format::column( "PAT", format_avg_pending_time),
                        terminal::format::column( "#B", format_branch_count)
                     );
                  }
               }

               constexpr auto resource_proxy_legend = R"(
name:
   name of the resource-proxy
id: 
   id of the resource proxy. `L-XX` if local and `E-XX` if _external_
key:
   the configured `key` of the resource
openinfo:
   configured openinfo for the resource
closeinfo:
   configured closeinfo for the resource
#B:
   number of branches currently associated with the resource-proxy
invoked:
   number of invocations to the resource-proxy
min:
   minimum-time - the minimum roundtrip time to the resource-proxy (in seconds)
max:
   maximum-time - the maximum roundtrip time to the resource-proxy (in seconds)
avg:
   average-time - the average roundtrip time to the resource-proxy (in seconds)
P:
   Pending - total number of pending request, over time.
PAT:
   Pending-Average-Time - the average time request has waited for a resource-proxy (in seconds)
   This only includes pending requests.
#:
   number of instances
)";


               auto internal_instances()
               {
                  auto format_pid = []( const admin::model::resource::Instance& value) 
                  {
                     return value.process.pid;
                  };

                  auto format_ipc = []( const admin::model::resource::Instance& value) 
                  {
                     return value.process.ipc;
                  };

                  auto format_invoked = []( const admin::model::resource::Instance& value) 
                  {
                     return value.metrics.roundtrip.count;
                  };

                  auto format_min = []( const admin::model::resource::Instance& value)
                  {
                     return std::chrono::duration_cast< time_type>( value.metrics.roundtrip.limit.min).count();
                  };

                  auto format_max = []( const admin::model::resource::Instance& value)
                  {
                     return std::chrono::duration_cast< time_type>( value.metrics.roundtrip.limit.max).count();
                  };

                  auto format_avg = []( const admin::model::resource::Instance& value)
                  {
                     if( value.metrics.roundtrip.count == 0) return 0.0;
                     return std::chrono::duration_cast< time_type>( value.metrics.roundtrip.total / value.metrics.roundtrip.count).count();
                  };

                  auto format_rm_invoked = []( const admin::model::resource::Instance& value)
                  {
                     return value.metrics.resource.count;
                  };

                  auto format_rm_min = []( const admin::model::resource::Instance& value)
                  {
                     return std::chrono::duration_cast< time_type>( value.metrics.resource.limit.min).count();
                  };

                  auto format_rm_max= []( const admin::model::resource::Instance& value)
                  {
                     return std::chrono::duration_cast< time_type>( value.metrics.resource.limit.max).count();
                  };

                  auto format_rm_avg = []( const admin::model::resource::Instance& value)
                  {
                     if( value.metrics.resource.count == 0) return 0.0;
                     return std::chrono::duration_cast< time_type>( value.metrics.resource.total / value.metrics.resource.count).count();
                  };

                  return common::terminal::format::formatter< admin::model::resource::Instance>::construct(
                     terminal::format::column( "id", std::mem_fn( &admin::model::resource::Instance::id), common::terminal::color::yellow, terminal::format::Align::right),
                     terminal::format::column( "pid", format_pid, common::terminal::color::white, terminal::format::Align::right),
                     terminal::format::column( "ipc", format_ipc, common::terminal::color::no_color, terminal::format::Align::right),
                     terminal::format::column( "invoked", format_invoked, common::terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "min", format_min, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "max", format_max, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "avg", format_avg, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rm-invoked", format_rm_invoked, common::terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rm-min", format_rm_min, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rm-max", format_rm_max, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rm-avg", format_rm_avg, terminal::color::blue, terminal::format::Align::right)
                  );
               }

               constexpr std::string_view internal_instances_legend = R"(
id:
   id of the instance
pid:
   process id of the instance
ipc:
   ipc of the instance
invoked:
   number of invocations to the instance
min:
   minimum-time - the minimum roundtrip time to the instance (in seconds)
max:
   maximum-time - the maximum roundtrip time to the instance (in seconds)
avg:
   average-time - the average roundtrip time to the instance (in seconds)
rm-invoked:
   number of invocations to the resource
rm-min:
   minimum-time - the minimum roundtrip time to the resource (in seconds)
rm-max:
   maximum-time - the maximum roundtrip time to the resource (in seconds)
rm-avg:
   average-time - the average roundtrip time to the resource (in seconds)
)";


               auto external_instances()
               {
                  auto format_id = []( auto& value) { return value.id;};
                  auto format_pid = []( auto& value) { return value.process.pid;};
                  auto format_ipc = []( auto& value) { return value.process.ipc;};
                  auto format_alias = []( auto& value) { return terminal::format::guard_empty( value.alias);};
                  auto format_description = []( auto& value) { return terminal::format::guard_empty( value.description);}; 

                  if( ! terminal::output::directive().porcelain())
                     return common::terminal::format::formatter< local::normalized::Instance>::construct(
                        common::terminal::format::column( "id", format_id, common::terminal::color::yellow, terminal::format::Align::left),
                        common::terminal::format::column( "alias", format_alias, common::terminal::color::blue, terminal::format::Align::left),
                        common::terminal::format::column( "pid", format_pid, common::terminal::color::no_color, terminal::format::Align::right),
                        common::terminal::format::column( "ipc", format_ipc, common::terminal::color::no_color, terminal::format::Align::right),
                        common::terminal::format::column( "description", format_description, common::terminal::color::yellow, terminal::format::Align::left)
                     );
                  else
                     return common::terminal::format::formatter< local::normalized::Instance>::construct(
                        common::terminal::format::column( "id", format_id),
                        common::terminal::format::column( "alias", format_alias),
                        common::terminal::format::column( "pid", format_pid),
                        common::terminal::format::column( "description", format_description),
                        common::terminal::format::column( "ipc", format_ipc)
                     );
               }

               constexpr std::string_view external_instances_legend = R"(
id:
   id of the instance
alias:
   alias of the instance
pid:
   process id of the instance
ipc:
   ipc of the instance
description:
   description of the instance, if any. If the instance is a gateway outbound
   this will hold the name of the other domain.
)";

               auto instances()
               {
                  struct format_state
                  {
                     std::size_t width( const normalized::Instance& instance, const std::ostream&) const
                     {
                        return description( instance.state).size();
                     }

                     void print( std::ostream& out, const normalized::Instance& instance, std::size_t width) const
                     {
                        out << std::setfill( ' ');

                        using State = normalized::instance::State;
                        switch( instance.state)
                        {
                           case State::unknown: stream::write( out, std::left, std::setw( width), terminal::color::red, State::unknown); break;
                           case State::spawned: stream::write( out, std::left, std::setw( width), terminal::color::white, State::spawned); break;
                           case State::idle: stream::write( out, std::left, std::setw( width), terminal::color::green, State::idle);break;
                           case State::busy: stream::write( out, std::left, std::setw( width), terminal::color::yellow, State::busy);break;
                           case State::shutdown: stream::write( out, std::left, std::setw( width), terminal::color::red, State::shutdown); break;
                           case State::external: stream::write( out, std::left, std::setw( width), terminal::color::cyan, State::external); break;
                        }
                     }
                  };

                  auto format_id = []( auto& value) { return value.id;};
                  auto format_pid = []( auto& value) { return value.process.pid;};
                  auto format_ipc = []( auto& value) { return value.process.ipc;};
                  auto format_alias = []( auto& value) { return terminal::format::guard_empty( value.alias);};
                  auto format_description = []( auto& value) { return terminal::format::guard_empty( value.description);};

                  if( ! terminal::output::directive().porcelain())
                  {
                     return terminal::format::formatter< local::normalized::Instance>::construct(
                        terminal::format::column( "id", format_id, terminal::color::yellow, terminal::format::Align::left),
                        terminal::format::column( "alias", format_alias, terminal::color::blue, terminal::format::Align::left),
                        terminal::format::custom::column( "state", format_state{}),
                        terminal::format::column( "pid", format_pid, terminal::color::no_color, terminal::format::Align::right),
                        terminal::format::column( "ipc", format_ipc, terminal::color::no_color, terminal::format::Align::right),
                        terminal::format::column( "description", format_description, terminal::color::yellow, terminal::format::Align::left)
                     );
                  }
                  else
                  {
                     return terminal::format::formatter< local::normalized::Instance>::construct(
                        terminal::format::column( "id", format_id),
                        terminal::format::custom::column( "state", format_state{}),
                        terminal::format::column( "pid", format_pid),
                        terminal::format::column( "alias", format_alias),
                        terminal::format::column( "description", format_description),
                        terminal::format::column( "ipc", format_ipc)
                     );
                  }
               }

               constexpr std::string_view instances_legend = R"(
id:
   id of the instance
alias:
   alias of the instance
state:
   state of the instance, one of: unknown, spawned, idle, busy, shutdown, external
pid:
   process id of the instance
ipc:
   ipc of the instance
description:
   description of the instance, if any. If the instance is a gateway outbound
   this will hold the name of the other domain.
)";

            } // format

            namespace list
            {
               namespace transactions
               {
                  auto option()
                  {
                     return argument::Option{
                        []()
                        {
                           auto state = call::state();
                           format::transaction().print( std::cout, state.transactions);
                        },
                        { "-lt", "--list-transactions"},
                        R"(list current transactions)"
                     };
                  }
               } // transactions

               namespace resources
               {
                  auto option()
                  {
                     return argument::Option{
                        []()
                        {
                           auto state = call::state();

                           const auto branch_count = normalized::compute_branch_count( state);

                           format::resource_proxy( branch_count).print( std::cout, algorithm::sort( state.resources));
                        },
                        { "-lr", "--list-resources" },
                        R"(list all resources)"
                     };
                  }
                  
               } // resources

               namespace resource::instances
               {
                  void internal()
                  {
                     auto transform = []( auto&& resources)
                     {
                        using instances_t = std::vector< admin::model::resource::Instance>;

                        return algorithm::accumulate( resources, instances_t{}, []( auto instances, auto& resource)
                        {
                           return algorithm::container::append( resource.instances, std::move( instances));
                        });
                     };

                     auto instances = transform( call::state().resources);
                     format::internal_instances().print( std::cout, algorithm::sort( instances));
                  }

                  void external()
                  {
                     auto is_external = []( auto& value){ return value.state == decltype( value.state)::external;};

                     auto instances = normalized::instances();

                     auto externals = algorithm::filter( instances, is_external);
                     format::external_instances().print( std::cout, externals);
                  }

                  enum struct Flag
                  {
                     none = 0,
                     internal = 1,
                     external = 2,
                     all = internal | external
                  };

                  [[maybe_unused]] consteval void casual_enum_as_flag( Flag){};


                  auto option()
                  {
                     struct Shared
                     {
                        Flag flag = Flag::none;
                     };

                     auto shared = std::make_shared< Shared>();

                     auto invoke = [ shared]()
                     {
                        common::log::debug( "shared->flag: ", shared->flag);

                        if( shared->flag == Flag::internal)
                        {
                           instances::internal();
                        }
                        else if( shared->flag == Flag::external)
                        {
                           instances::external();
                        }
                        else // Flag::all (or none)
                        {
                           auto instances = normalized::instances();
                           format::instances().print( std::cout, instances);
                        }
                     };

                     auto create_suboption = [ shared]( Flag flag, std::vector< std::string> names, std::string description)
                     {  
                        return argument::Option{
                           [ flag, shared]() 
                           { 
                              shared->flag |= flag; 
                              // we return preemptive to make sure this suboption is invoked before the main invoke
                              return argument::option::invoke::preemptive{};
                           },
                           std::move( names),
                           std::move( description)
                        };
                     };

                     return argument::Option{
                        std::move( invoke),
                        argument::option::Names{ { "-lri", "--list-resource-instances"}, { "-li", "--list-instances"}},
                        R"(list resource instances)"
                     }({
                        create_suboption( Flag::all, { "-a", "--all"}, "list both internal and external resource instances (default)"),
                        create_suboption( Flag::internal, { "-i", "--internal"}, "list internal resource instances"),
                        create_suboption( Flag::external, { "-e", "--external"}, "list external resource instances")
                     });
                  }

               } // resource::instances

               namespace deprecated
               {
                  namespace external::instances
                  {
                     auto option()
                     {
                        return argument::Option{
                           &resource::instances::external,
                           argument::option::Names( {}, { "--list-external-instances", "--list-external-resources" }),
                           R"(@deprecated: use --list-resource-instances --external)"
                        };
                     }
                  } // external::instances

                  namespace internal::instances
                  {
                     auto option()
                     {
                        return argument::Option{
                           &resource::instances::internal,
                           argument::option::Names{ {}, { "--list-internal-instances" }},
                           R"(@deprecated: use --list-resource-instances --internal)"
                        };
                     }
                  } // internal::instances
               } // deprecated

               namespace pending
               {
                  auto option()
                  {
                     return argument::Option{
                        []()
                        {
                           auto state = call::state();

                           auto debug = common::serialize::log::writer();

                           debug << CASUAL_NAMED_VALUE( state.pending);
                           debug << CASUAL_NAMED_VALUE( state.log);

                           debug.consume( std::cout);
                        },
                        { "-lp", "--list-pending" },
                        R"(list pending tasks)"};
                  }
               } // pending
            } // list

            namespace scale::resource::proxies
            {
               auto option()
               {
                  auto invoke = []( std::vector< std::tuple< std::string, int>> values)
                  {
                     call::scale::resource::proxy::instances( common::algorithm::transform( values, []( auto& value){
                        if( std::get< 1>( value) < 0)
                           code::raise::error( code::casual::invalid_argument, "number of instances cannot be negative");

                        return admin::model::scale::resource::proxy::Instances{ 
                           .name = std::get< 0>( value),
                           .instances = std::get< 1>( value)
                        };
                     }));
                  };

                  auto completer = []( bool help, auto values) -> std::vector< std::string>
                  {
                     if( help)
                        return { "rm-id", "#instances"};

                     if( values.size() % 2 == 0)
                        return algorithm::transform( call::state().resources, []( auto& r){ return r.name;});

                     return { std::string{ argument::reserved::name::suggestions}}; 
                  };

                  return argument::Option{
                     std::move( invoke), 
                     std::move( completer),
                     argument::option::Names( { "-srp", "--scale-resource-proxies"}, { "-si", "--scale-instances"}),
                     R"(scale resource proxy instances)"
                  };
               }

            } // scale::resource::proxies

            namespace information
            {
               auto call() -> std::vector< std::tuple< std::string, std::string>> 
               {
                  auto state = local::call::state();

                  auto accumulate = []( auto extract)
                  {
                     return [extract]( auto& queues)
                     {
                        decltype( extract( range::front( queues))) initial{};
                        return algorithm::accumulate( queues, initial, [extract]( auto count, auto& queue){ return count + extract( queue);});
                     };
                  };

                  auto instance_count = [accumulate,&state]()
                  {
                     return accumulate( []( auto& resource){ return resource.instances.size();})( state.resources);
                  };

                  using second_t = std::chrono::duration< double>;

                  auto metric_resource_count = [accumulate]( auto& resources)
                  {
                     return accumulate( []( auto& resource){ return resource.metrics.resource.count;})( resources);
                  };

                  auto metric_resource_total = [accumulate]( auto& resources) -> second_t
                  {
                     return accumulate( []( auto& resource){ return resource.metrics.resource.total;})( resources);
                  };

                  auto metric_roundtrip_count = [accumulate]( auto& resources)
                  {
                     return accumulate( []( auto& resource){ return resource.metrics.roundtrip.count;})( resources);
                  };

                  auto metric_roundtrip_total = [accumulate]( auto& resources) -> second_t
                  {
                     return accumulate( []( auto& resource){ return resource.metrics.roundtrip.total;})( resources);
                  };


                  auto average = [=]( auto total, auto count) -> second_t
                  {
                     if( count == 0)
                        return {};

                     return std::chrono::duration_cast< second_t>( total / count);
                  };
                  
                  return {
                     { "transaction.manager.resource.count", string::compose( state.resources.size())},
                     { "transaction.manager.resource.instance.count", string::compose( instance_count())},
                     { "transaction.manager.resource.metrics.resource.count", string::compose( metric_resource_count( state.resources))},
                     { "transaction.manager.resource.metrics.resource.total", string::compose( metric_resource_total( state.resources))},
                     { "transaction.manager.resource.metrics.resource.average", 
                        string::compose( average( metric_resource_total( state.resources), metric_resource_count( state.resources)))},
                     { "transaction.manager.resource.metrics.roundtrip.count", string::compose( metric_roundtrip_count( state.resources))},
                     { "transaction.manager.resource.metrics.roundtrip.total", string::compose( metric_roundtrip_total( state.resources))},
                     { "transaction.manager.resource.metrics.roundtrip.average", 
                        string::compose( average( metric_roundtrip_total( state.resources), metric_roundtrip_count( state.resources)))},

                     { "transaction.manager.log.writes", string::compose( state.log.writes)},
                     { "transaction.manager.log.update.prepare", string::compose( state.log.update.prepare)},
                     { "transaction.manager.log.update.remove", string::compose( state.log.update.remove)},
                  };
               }

               auto option()
               {
                  return argument::Option{
                     [](){ terminal::formatter::key::value().print( std::cout, call());},
                     { "--information"},
                     R"(collect aggregated information about transactions in this domain)"
                  };
               }

            } // information


            namespace begin
            {
               auto option()
               {
                  auto invoke = []()
                  {
                     Trace trace{ "transaction::manager::admin::local::handle::begin::invoke"};

                     // create transaction and send it downstream
                     {
                        casual::cli::message::transaction::Current message;
                        message.trid = common::transaction::id::create();

                        casual::cli::pipe::forward::message( message);
                     }

                     communication::stream::inbound::Device in{ std::cin};

                     casual::cli::pipe::done::Scope done;

                     auto handler = common::message::dispatch::handler( in, 
                        casual::cli::pipe::forward::handle::defaults(),
                        std::ref( done)
                     );

                     // start the pump, and forward all upstream messages
                     common::message::dispatch::pump( casual::cli::pipe::condition::done( done), handler, in);

                     // done dtor will send Done downstream

                  };

                  return argument::Option{
                     std::move( invoke),
                     { "--begin"},
                     R"(creates a 'single' transaction directive

* creates a new transaction and send it downstream.
* all downstream `actions` will be associated with this transaction, until commit/rollback.
* @attention there has to be a corresponding commit/rollback downstream for every 
--begin, otherwise the transaction(s) will be unresolved (indoubt).

@note: part of casual-pipe
)"
                  };
               }

            } // begin

            namespace handle
            {
               struct State
               {
                  common::transaction::ID current;
                  casual::cli::pipe::done::Scope done;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( current);
                     CASUAL_SERIALIZE( done);
                  )
               };


               template< typename C>
               auto consume( State& state, C&& callback)
               {
                  Trace trace{ "transaction::manager::admin::local::handle::consume"};

                  communication::stream::inbound::Device in{ std::cin};

                  // first we wait for the transaction::Current message (and done), and invoke the 
                  // callback with it. This to ensure that we commit/rollback before we send
                  // other stuff downstream.
                  {
                     // snatch the first transaction, and forward the rest (if any)
                     auto transcation_current = [ &state]( casual::cli::message::transaction::Current& current)
                     {
                        if( state.current)
                           casual::cli::pipe::forward::message( current);
                        else
                              state.current = current.trid;
                     };

                     auto handler = common::message::dispatch::handler( in, 
                        std::move( transcation_current),
                        std::ref( state.done));

                     // we start a relaxed pump, to consume all other messages to the "device cache".
                     common::message::dispatch::relaxed::pump( casual::cli::pipe::condition::done( state.done), handler, in);

                     callback( state);
                  }
                  
                  // we need to send the stuff we have in the "device cache" downstream.
                  auto handler = common::message::dispatch::handler( in, casual::cli::pipe::forward::handle::defaults());

                  while( handler( communication::device::non::blocking::next( in)))
                     ; // no-op
               }
               
            } // handle

            namespace rollback
            {
               void apply( const common::transaction::ID& trid)
               {
                  common::message::transaction::rollback::Request request{ process::handle()};
                  request.trid = trid;

                  auto reply = communication::ipc::call( communication::instance::outbound::transaction::manager::device(), request);

                  if( reply.state != decltype( reply.state)::ok)                           
                     casual::cli::pipe::log::error( reply.state, " failed to rollback trid: ", trid);
               }

               auto option()
               {
                  auto invoke = []()
                  {
                     Trace trace{ "transaction::manager::admin::local::rollback::invoke"};

                     handle::State state;

                     handle::consume( state, []( const handle::State& state)
                     {
                        rollback::apply( state.current);
                     });

                     // dtor in state.done will send done downstream
                  };

                  return argument::Option{
                     std::move( invoke),
                     { "--rollback"},
                     R"(tries to rollback the upstream transaction

* The current transaction will be rolled back.
* Downstream `actions` will not be associated with the current transaction.

@note: part of casual-pipe
)"
                  };
               }
            } // rollback

            namespace commit
            {
               auto option()
               {
                  auto invoke = []()
                  {
                     Trace trace{ "transaction::manager::admin::local::commit::invoke"};

                        handle::State state;

                     handle::consume( state, []( const handle::State& state)
                     {  
                        Trace trace{ "transaction::manager::admin::local::commit::invoke consume"};
                        log::debug( "state: ", state); 

                        // check if we got errors upstream, and need to rollback.
                        if( state.done.pipe_error())
                        {
                           casual::cli::pipe::log::error( "transaction in rollback only state - rollback trid: ", state.current);
                           rollback::apply( state.current);
                           return;
                        }

                        common::message::transaction::commit::Request request{ process::handle()};
                        request.trid = state.current;

                        auto reply = communication::ipc::call( communication::instance::outbound::transaction::manager::device(), request);
                        
                        // If we get the persistent prepare, we need to wait for the final commit-reply
                        if( reply.stage == decltype( reply.stage)::prepare)
                           reply = communication::ipc::receive< common::message::transaction::commit::Reply>( reply.correlation);

                        if( reply.state != decltype( reply.state)::ok)
                           code::raise::error( reply.state);
                     });

                     // dtor in state.done will send done downstream
                  };

                  return argument::Option{
                     std::move( invoke),
                     { "--commit"},
                        R"(tries to commit the upstream transaction

* The current transaction will be committed (if error from upstream -> rollback)
* Downstream `actions` will not be associated with the current transaction.

@note: part of casual-pipe
)"
                  };
               }

            } // commit

            namespace legend
            {
               auto option()
               {

                  static constexpr auto legend_option = [](  std::string key, std::string_view legend)
                  {
                     return argument::Option{ [ key, legend]()
                        {
                           std::cout << legend;
                        },
                        { key},
                        string::compose( "list legend for ", key)
                     };
                  };

                  auto list_resource_instances = argument::Option{ 
                        [](){},
                        { "--list-resource-instances"},
                        R"(the legends for list resource instances suboptions

The following suboptions has legend:
)"
                     }({
                        legend_option( "--all", local::format::instances_legend),
                        legend_option( "--internal", local::format::internal_instances_legend),
                        legend_option( "--external", local::format::external_instances_legend)
                     });

                  return argument::Option{
                     [](){},
                     { "--legend"},
                     R"(the legend for the supplied option

Documentation and description for abbreviations and acronyms used as columns in output

The following options has legend:
)"
                  }({
                     legend_option( "--list-resources", local::format::resource_proxy_legend),
                     legend_option( "--list-transactions", local::format::transactions_legend),
                     std::move( list_resource_instances),
                  });
               }

            } // legend

         } // <unnamed>
      } // local

      argument::Option options()
      {
         return argument::Option{ [](){}, { "transaction"}, "transaction related administration"}( {
            local::list::transactions::option(),
            local::list::resources::option(),
            local::list::resource::instances::option(),
            local::begin::option(),
            local::commit::option(),
            local::rollback::option(),
            local::scale::resource::proxies::option(),
            local::list::pending::option(),
            local::legend::option(),
            local::information::option(),
            casual::cli::state::option( &local::call::state),
            local::list::deprecated::internal::instances::option(),
            local::list::deprecated::external::instances::option(),
         });
      }

      std::vector< std::tuple< std::string, std::string>> information()
      {
         return local::information::call();
      }

   } // transaction::manager::admin::cli
} // casual

