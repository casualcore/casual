//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/manager/admin/cli.h"
#include "transaction/manager/admin/model.h"
#include "transaction/manager/admin/server.h"


#include "common/argument.h"
#include "common/environment.h"
#include "common/terminal.h"
#include "common/exception/capture.h"
#include "common/serialize/create.h"
#include "common/communication/instance.h"
#include "common/communication/ipc.h"
#include "common/message/dispatch/handle.h"
#include "common/range/adapter.h"
#include "common/algorithm/sorted.h"

#include "serviceframework/service/protocol/call.h"

#include "casual/cli/pipe.h"
#include "casual/cli/state.h"


namespace casual
{
   using namespace common;

   namespace transaction
   {
      namespace manager
      {

         namespace admin
         {
            namespace local
            {
               namespace
               {
                  namespace call
                  {
                     auto state()
                     {
                        serviceframework::service::protocol::binary::Call call;
                        auto reply = call( service::name::state());

                        admin::model::State result;

                        reply >> CASUAL_NAMED_VALUE( result);

                        return result;
                     }

                     namespace scale
                     {
                        auto instances( const std::vector< admin::model::scale::Instances>& instances)
                        {
                           serviceframework::service::protocol::binary::Call call;

                           call << CASUAL_NAMED_VALUE( instances);
                           auto reply = call( service::name::scale::instances());

                           std::vector< admin::model::resource::Proxy> result;

                           reply >> CASUAL_NAMED_VALUE( result);

                           return result;
                        }
                     } // update

                  } // call

                  namespace external
                  {
                     using base_type = admin::model::resource::external::Proxy;
                     struct Proxy : base_type
                     {
                        Proxy() = default;
                        Proxy( base_type base, std::string alias): base_type{ base}, alias{ std::move( alias)} { }

                        std::string alias;
                     };
                  } // external

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

                        auto transform_metric = []( auto& v)
                        {  
                           common::Metric result;
                           result.count = v.count;
                           result.total = v.total;
                           result.limit.min = v.limit.min;
                           result.limit.max = v.limit.max;
                           return result;
                        };

                        Metrics result;
                        result.resource = transform_metric( value.metrics.resource);
                        result.roundtrip = transform_metric( value.metrics.roundtrip);

                        for( auto& instance : value.instances)
                        {
                           result.resource += transform_metric( instance.metrics.resource);
                           result.roundtrip += transform_metric( instance.metrics.roundtrip);
                        }
                        return result;
                     }

                     auto transaction()
                     {
                        auto format_global = []( auto& value) { return value.global.id;};
                        auto format_number_of_branches = []( auto& value) { return value.branches.size();};
                        auto format_owner = []( auto& value) -> std::string
                        {
                           if( value.owner.pid)
                              return string::compose( value.owner.pid);
                           else
                              return "-";
                        };

                        auto format_stage = []( auto& value)
                        {
                           return value.stage;
                        };

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

                        return common::terminal::format::formatter< admin::model::Transaction>::construct(
                           common::terminal::format::column( "global", format_global, common::terminal::color::yellow),
                           common::terminal::format::column( "#branches", format_number_of_branches, common::terminal::color::no_color),
                           common::terminal::format::column( "owner", format_owner, common::terminal::color::white, common::terminal::format::Align::right),
                           common::terminal::format::column( "stage", format_stage, common::terminal::color::green, common::terminal::format::Align::left),
                           common::terminal::format::column( "resources", format_resources, common::terminal::color::magenta, common::terminal::format::Align::left)
                        );
                     }

                     auto resource_proxy()
                     {

                        struct format_number_of_instances
                        {
                           std::size_t operator() ( const admin::model::resource::Proxy& value) const
                           {
                              return value.instances.size();
                           }
                        };

                        auto format_invoked = []( const admin::model::resource::Proxy& value)
                        {
                           auto result = accumulate_statistics( value);
                           return result.roundtrip.count;
                        };

                        auto format_min = []( const admin::model::resource::Proxy& value)
                        {
                           auto result = accumulate_statistics( value);
                           return std::chrono::duration_cast< time_type>( result.roundtrip.limit.min).count();
                        };

                        auto format_max = []( const admin::model::resource::Proxy& value)
                        {
                           auto result = accumulate_statistics( value);
                           return std::chrono::duration_cast< time_type>( result.roundtrip.limit.max).count(); 
                        };

                        auto format_avg = []( const admin::model::resource::Proxy& value)
                        {
                           auto result = accumulate_statistics( value);
                           if( result.roundtrip.count == 0) return 0.0;
                           return std::chrono::duration_cast< time_type>( result.roundtrip.total / result.roundtrip.count).count();
                        };


                        return common::terminal::format::formatter< admin::model::resource::Proxy>::construct(
                           common::terminal::format::column( "name", std::mem_fn( &admin::model::resource::Proxy::name), common::terminal::color::yellow),
                           common::terminal::format::column( "id", std::mem_fn( &admin::model::resource::Proxy::id), common::terminal::color::yellow, terminal::format::Align::right),
                           common::terminal::format::column( "key", std::mem_fn( &admin::model::resource::Proxy::key), common::terminal::color::yellow),
                           common::terminal::format::column( "openinfo", std::mem_fn( &admin::model::resource::Proxy::openinfo), common::terminal::color::no_color),
                           common::terminal::format::column( "closeinfo", std::mem_fn( &admin::model::resource::Proxy::closeinfo), common::terminal::color::no_color),
                           terminal::format::column( "invoked", format_invoked, terminal::color::blue, terminal::format::Align::right),
                           terminal::format::column( "min (s)", format_min, terminal::color::blue, terminal::format::Align::right),
                           terminal::format::column( "max (s)", format_max, terminal::color::blue, terminal::format::Align::right),
                           terminal::format::column( "avg (s)", format_avg, terminal::color::blue, terminal::format::Align::right),
                           terminal::format::column( "#", format_number_of_instances{}, terminal::color::white, terminal::format::Align::right)
                        );

                     }

                     auto external_resource()
                     {
                        auto format_pid = []( const local::external::Proxy& value) 
                        {
                           return value.process.pid;
                        };

                        return common::terminal::format::formatter< local::external::Proxy>::construct(
                           common::terminal::format::column( "id", std::mem_fn( &local::external::Proxy::id), common::terminal::color::magenta, terminal::format::Align::left),
                           common::terminal::format::column( "alias", std::mem_fn( &local::external::Proxy::alias), common::terminal::color::no_color, terminal::format::Align::left),
                           common::terminal::format::column( "pid", format_pid, common::terminal::color::no_color, terminal::format::Align::right)
                        );
                     }

                     auto resource_instance()
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
                           terminal::format::column( "min (s)", format_min, terminal::color::blue, terminal::format::Align::right),
                           terminal::format::column( "max (s)", format_max, terminal::color::blue, terminal::format::Align::right),
                           terminal::format::column( "avg (s)", format_avg, terminal::color::blue, terminal::format::Align::right),
                           terminal::format::column( "rm-invoked", format_rm_invoked, common::terminal::color::blue, terminal::format::Align::right),
                           terminal::format::column( "rm-min (s)", format_rm_min, terminal::color::blue, terminal::format::Align::right),
                           terminal::format::column( "rm-max (s)", format_rm_max, terminal::color::blue, terminal::format::Align::right),
                           terminal::format::column( "rm-avg (s)", format_rm_avg, terminal::color::blue, terminal::format::Align::right)
                        );

                     }
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
                                 format::resource_proxy().print( std::cout, algorithm::sort( state.resources));
                              },
                              { "-lr", "--list-resources" },
                              R"(list all resources)"
                           };
                        }
                        
                     } // resources

                     namespace external
                     {
                        auto option()
                        {
                           return argument::Option{ []()
                              {
                                 auto transform = []( auto&& externals)
                                 {
                                    auto request = message::domain::process::information::Request{ process::handle()};
                                    request.handles = algorithm::transform( externals, []( const auto& e) { return e.process;});

                                    // we query the DM for more information about the externals
                                    auto processes = communication::ipc::call( communication::instance::outbound::domain::manager::device(), request).processes;

                                    return algorithm::accumulate( processes, std::vector< local::external::Proxy>{}, [&externals]( auto result, auto& process)
                                    {
                                       auto external = algorithm::find_if( externals, [&process]( const auto& external) {
                                          return external.process.pid == process.handle;
                                       });

                                       if( external)
                                          result.emplace_back( *external, process.alias);

                                       return result;
                                    });
                                 };

                                 auto externals = transform( call::state().externals);
                                 format::external_resource().print( std::cout, externals);
                              },
                              { "--list-external-resources" },
                              R"(list external resources)"
                           };
                        }
                     } // external

                     namespace instances
                     {
                        auto option()
                        {
                           return argument::Option{
                              []()
                              {
                                 auto transform = []( auto&& resources)
                                 {
                                    using instances_t = std::vector< admin::model::resource::Instance>;

                                    return algorithm::accumulate( resources, instances_t{}, []( auto instances, auto& resource)
                                    {
                                       return algorithm::append( resource.instances, std::move( instances));
                                    });
                                 };

                                 auto instances = transform( call::state().resources);
                                 format::resource_instance().print( std::cout, algorithm::sort( instances));
                              },
                              { "-li", "--list-instances"},
                              R"(list resource instances)"
                           };
                        }
                     } // instances

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

                  namespace scale
                  {
                     namespace instances
                     {
                        auto option()
                        {
                           return argument::Option{
                              [](  std::vector< std::tuple< std::string, int>> values)
                              {
                                 auto resources = call::scale::instances( common::algorithm::transform( values, []( auto& value){
                                    if( std::get< 1>( value) < 0)
                                       code::raise::error( code::casual::invalid_argument, "number of instances cannot be negative");
                                    
                                    admin::model::scale::Instances instance;
                                    instance.name = std::get< 0>( value);
                                    instance.instances = std::get< 1>( value);
                                    return instance;
                                 }));

                                 format::resource_proxy().print( std::cout, algorithm::sort( resources));
                              },
                              []( auto values, bool help) -> std::vector< std::string>
                              {
                                 if( help)
                                    return { "rm-id", "# instances"};

                                 if( values.size() % 2 == 0)
                                    return algorithm::transform( call::state().resources, []( auto& r){ return r.name;});
            
                                 return { common::argument::reserved::name::suggestions::value()}; 
                              },
                              { "-si", "--scale-instances"},
                              R"(scale resource proxy instances)"
                           };
                        }

                     } // instances
                  } // scale

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

                              cli::pipe::forward::message( message);
                           }

                           communication::stream::inbound::Device in{ std::cin};

                           cli::pipe::done::Scope done;

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
                           auto transcation_current = [ &state]( cli::message::transaction::Current& current)
                           {
                              if( state.current)
                                 cli::pipe::forward::message( current);
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
                           cli::pipe::log::error( reply.state, " failed to rollback trid: ", trid);
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
                              log::line( verbose::log, "state: ", state); 

                              // check if we got errors upstream, and need to rollback.
                              if( state.done.pipe_error())
                              {
                                 cli::pipe::log::error( "transaction in rollback only state - rollback trid: ", state.current);
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
               } // <unnamed>
            } // local

            struct CLI::Implementation
            {
               argument::Group options()
               {
                  return argument::Group{ [](){}, { "transaction"}, "transaction related administration",
                     local::list::transactions::option(),
                     local::list::resources::option(),
                     local::list::instances::option(),
                     local::list::external::option(),
                     local::begin::option(),
                     local::commit::option(),
                     local::rollback::option(),
                     local::scale::instances::option(),
                     local::list::pending::option(),
                     local::information::option(),
                     casual::cli::state::option( &local::call::state),
                  };
               }
            };

            CLI::CLI() = default; 
            CLI::~CLI() = default; 

            common::argument::Group CLI::options() &
            {
               return m_implementation->options();
            }

            std::vector< std::tuple< std::string, std::string>> CLI::information() &
            {
               return local::information::call();
            }

         } // admin
      } // manager
   } // transaction
} // casual

