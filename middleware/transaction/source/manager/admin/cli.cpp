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
                        auto format_owner = []( auto& value){ return value.owner.pid;};

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
                           common::terminal::format::column( "#branches", format_number_of_branches, common::terminal::color::grey),
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
                           common::terminal::format::column( "id", std::mem_fn( &local::external::Proxy::id), common::terminal::color::magenta, terminal::format::Align::right),
                           common::terminal::format::column( "alias", std::mem_fn( &local::external::Proxy::alias), common::terminal::color::no_color, terminal::format::Align::right),
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

                  namespace state
                  {
                     auto option()
                     {
                        auto complete = []( auto values, bool)
                        {
                           return std::vector< std::string>{ "json", "yaml", "xml", "ini"};
                        };

                        return argument::Option{
                           []( const std::optional< std::string>& format)
                           {
                              auto state = call::state();
                              auto archive = common::serialize::create::writer::from( format.value_or( "yaml"));
                              archive << CASUAL_NAMED_VALUE( state);
                              archive.consume( std::cout);
                           },
                           complete,
                           { "--state"},
                           R"(view current state in optional format)"
                        };
                     }
                  } // state

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
                           return argument::Option{
                              []()
                              {
                                 auto state = call::state();

                                 auto transform = []( auto&& externals)
                                 {
                                    return algorithm::accumulate( externals, std::vector< local::external::Proxy>{}, []( auto result, auto& external)
                                    {
                                       std::string alias = communication::ipc::call( external.process.ipc, message::domain::instance::global::state::Request{ process::handle()}).instance.alias; 

                                       result.emplace_back( external, alias);
                                       return result;
                                    });
                                 };

                                 auto externals = transform( state.externals);
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

                  namespace handle
                  {
                     namespace terminate
                     {
                        auto directive()
                        {
                           return []( const cli::message::transaction::Directive& message)
                           {
                              Trace trace{ "transaction::manager::admin::local::handle::terminate::directive"};

                              // terminate the upstream transaction directive
                              communication::device::blocking::optional::send( message.process.ipc, 
                                 cli::message::transaction::directive::Terminated{ common::process::handle()});

                           };
                        }
                        
                     } // terminate
                  } // handle


                  namespace begin
                  {
                     struct State
                     {
                        State( std::vector< common::transaction::ID> pending) : m_pending{ std::move( pending)} {}

                        enum class Machine : short
                        {
                           done,
                           pending,
                        };

                        friend std::ostream& operator << ( std::ostream& out, const Machine& value)
                        {
                           switch( value)
                           {
                              case Machine::done: return out << "done";
                              case Machine::pending: return out << "pending";
                           }
                           return out << "<unknown>";
                        }

                        bool done() const { return m_machine == Machine::done;}
                        bool operator () () const { return done();}

                        void terminate()
                        {
                           if( m_pending.empty())
                              m_machine = Machine::done;
                        }

                        void consume( const common::transaction::ID& trid)
                        {
                           algorithm::container::trim( m_pending, algorithm::remove( m_pending, trid));
                           if( m_pending.empty())
                              m_machine = Machine::done;
                        }

                        void add( const std::vector< common::transaction::ID>& pending) { algorithm::append( pending, m_pending);}
                        void add( const common::transaction::ID& pending) { m_pending.push_back( pending);}

                        CASUAL_LOG_SERIALIZE(
                           CASUAL_SERIALIZE_NAME( m_machine, "machine");
                           CASUAL_SERIALIZE_NAME( m_pending, "pending");
                        )

                     private:
                        Machine m_machine = Machine::pending;
                        std::vector< common::transaction::ID> m_pending;
                     };


                     namespace handle
                     {
                        template< typename A>
                        auto upstream( A&& associate)
                        {
                           Trace trace{ "transaction::manager::admin::local::begin::handle::upstream"};

                           // handle upstream, associate and forward, if any
                           {
                              bool done = false;
                        
                              communication::stream::inbound::Device in{ std::cin};

                              auto handler = common::message::dispatch::handler( in, 
                                 casual::cli::pipe::forward::handle::defaults(),
                                 casual::cli::pipe::handle::associate( associate),
                                 local::handle::terminate::directive(),
                                 casual::cli::pipe::handle::done( done)
                              );

                              // start the pump, and associate all relevant messages
                              common::message::dispatch::pump( casual::cli::pipe::condition::done( done), handler, in);
                           };

                           // send our directive downstream
                           {  
                              communication::stream::outbound::Device out{ std::cout};
                              communication::device::blocking::send( out, associate.directive);
                           }

                           // we're done in a 'casual-pipe' way.
                           casual::cli::pipe::done();

                           // we need to keep track of associated transactions
                           return State{ std::move( associate.associated)};
                        };

                        namespace downstream
                        {
                           auto finalize( State& state)
                           {
                              return [&state]( casual::cli::message::transaction::Finalize& message)
                              {
                                 Trace trace{ "transaction::manager::admin::local::begin::handle::downstream::finalize"};
                                 log::line( verbose::log, "message: ", message);

                                 // TODO error - check if our associations is a subset of the message.

                                 auto finalize_request = []( auto& transaction)
                                 {
                                    using Code = decltype( transaction.code);

                                    auto& tm = communication::instance::outbound::transaction::manager::device();

                                    if( transaction.code == Code::ok)
                                    {
                                       common::message::transaction::commit::Request request{ process::handle()};
                                       request.trid = transaction.trid;
                                       communication::device::blocking::optional::send( tm, request);
                                    }
                                    else 
                                    {
                                       common::message::transaction::rollback::Request request{ process::handle()};
                                       request.trid = transaction.trid;
                                       communication::device::blocking::optional::send( tm, request);
                                    }

                                    return transaction.trid;
                                 };

                                 state.add( algorithm::transform( message.transactions, finalize_request));
                                 log::line( verbose::log, "state: ", state);

                              };
                           }

                           auto terminated( State& state)
                           {
                              return [&state]( const casual::cli::message::transaction::directive::Terminated& message)
                              {
                                 Trace trace{ "transaction::manager::admin::local::begin::handle::downstream::terminated"};
                                 log::line( verbose::log, "message: ", message);

                                 state.terminate();
                                 log::line( verbose::log, "state: ", state);
                              };
                           }
                        } // downstream


                        namespace reply
                        {
                           auto commit( State& state)
                           {
                              return [&state]( common::message::transaction::commit::Reply& message)
                              {
                                 Trace trace{ "transaction::manager::admin::local::begin::handle::reply::commit"};
                                 log::line( verbose::log, "message: ", message);
                                 
                                 switch( message.stage)
                                 {
                                    using Enum = decltype( message.stage);
                                    case Enum::prepare: 
                                       return; // we wait for the next one
                                       break;
                                    case Enum::rollback:
                                    {
                                       casual::cli::pipe::log::error( "transaction commit failed - rolled back: ", message.trid);
                                       break;
                                    }
                                    case Enum::commit:
                                       break;
                                 }
                                 
                                 // consume the pending
                                 state.consume( message.trid);
                                 log::line( verbose::log, "state: ", state);
                              };
                           }

                           auto rollback( State& state)
                           {
                              return [&state]( common::message::transaction::rollback::Reply& message)
                              {
                                 Trace trace{ "transaction::manager::admin::local::begin::handle::reply::rollback"};
                                 log::line( verbose::log, "message: ", message);

                                 switch( message.stage)
                                 {
                                    using Enum = decltype( message.stage);
                                    case Enum::error:
                                    {
                                       casual::cli::pipe::log::error( "transaction rollback error: ", message.trid);
                                       break;
                                    }
                                    case Enum::rollback:
                                       break;
                                 }

                                 // consume the pending
                                 state.consume( message.trid);
                                 log::line( verbose::log, "state: ", state);
                              };
                           }
                        } // reply

                        auto associate( State& state)
                        {
                           return [&state]( const cli::message::transaction::Associated& message)
                           {
                              Trace trace{ "transaction::manager::admin::local::begin::handle::associate"};
                              common::log::line( verbose::log, "message: ", message);

                              state.add( message.trid);
                              log::line( verbose::log, "state: ", state);
                           };
                        }

                        
                     } // handle

                     template< typename A>
                     auto invoke( A&& associate)
                     {
                        return [associate = std::move( associate)]() mutable
                        {
                           Trace trace{ "transaction::manager::admin::local::begin::invoke"};

                           if( casual::cli::pipe::terminal::out())
                           {
                              casual::cli::pipe::log::error( "stdout is terminal - action: don't 'begin' transaction");
                              return;
                           }
                           
                           // handle upstream messages, associate and forward, if any. 
                           // send 'done' downstream
                           auto state = handle::upstream( std::move( associate));

                           // wait for downstream finalize, and/or terminate

                           auto& device = communication::ipc::inbound::device();

                           auto handler = common::message::dispatch::handler( device,
                              common::message::dispatch::handle::defaults(),
                              begin::handle::downstream::finalize( state),
                              begin::handle::downstream::terminated( state),
                              begin::handle::reply::commit( state),
                              begin::handle::reply::rollback( state),
                              begin::handle::associate( state));

                           auto condition = common::message::dispatch::condition::compose(
                              common::message::dispatch::condition::done( [&state](){ return state.done();}));

                           common::message::dispatch::pump( condition, handler, device);
                        };  
                     }
              

                     auto option()
                     {
                        return argument::Option{
                           begin::invoke( casual::cli::pipe::transaction::association::single()),
                           { "--begin"},
                           R"(creates a 'single' transaction directive

* associates all upstream transaction aware messages with the 'single' transaction, if they aren't associated already.
* all directives from upstream will be 'terminated', that is, notify the upstream 'owner' and not forward the directive
* sends the directive downstream, so other casual-pipe components can associate 'new stuff' with the 'single' transaction

hence, only one 'directive' can be in flight within a link in the casual-pipe.
)"
                        };
                     }

                     namespace compound
                     {
                        auto option()
                        {
                           return argument::Option{
                              begin::invoke( casual::cli::pipe::transaction::association::compound()),
                              { "--compound"},
                              R"(creates a compound transaction directive 

* associates all upstream transaction aware messages with a new transaction, if they aren't associated already.
* all directives from upstream will be 'terminated', that is, notify the upstream 'owner' and not forward the directive
* sends the directive downstream, so other casual-pipe components can associate 'new stuff' with a new transaction

hence, only one 'directive' can be in flight within a link in the casual-pipe.
)"
                           };
                        }
                     } // compound
 
                  } // begin

                  namespace handle
                  {
                     namespace predicate
                     {
                        namespace owner
                        {
                           auto less = []( auto& l, auto& r){ return l.trid.owner() < r.trid.owner();};
                        } // owner
                     } // predicate

                     namespace detail
                     {
                        void update( std::vector< cli::message::Transaction>& transactions, cli::message::Transaction&& source)
                        {
                           if( source)
                           {
                              // do we got it before?
                              if( auto found = algorithm::find( transactions, source))
                              {
                                 // is the new one in an 'error state'? if so, use that... 
                                 if( ! source.committable())
                                    *found = std::move( source);
                              }
                              else
                                 transactions.push_back( std::move( source));
                           }

                        }

                        template< typename M>
                        auto accumulate( std::vector< cli::message::Transaction>& transactions)
                        {
                           return [&transactions]( M& message)
                           {
                              detail::update( transactions, std::exchange( message.transaction, {}));
                              cli::pipe::forward::message( message);
                           };
                        }
                        
                     } // detail

                     auto accumulate()
                     {
                        Trace trace{ "transaction::manager::admin::local::handle::accumulate"};

                        bool done = false;
                        std::vector< cli::message::Transaction> result;
                        communication::stream::inbound::Device in{ std::cin};

                        auto handler = common::message::dispatch::handler( in, 
                           casual::cli::pipe::forward::handle::defaults(),
                           casual::cli::pipe::handle::done( done),
                           local::handle::terminate::directive(),
                           detail::accumulate< cli::message::Payload>( result),
                           detail::accumulate< cli::message::queue::message::ID>( result),
                           [&result]( cli::message::transaction::Propagate& message)
                           {
                              Trace trace{ "transaction::manager::admin::local::handle::accumulate Propagate"};

                              detail::update( result, std::move( message.transaction));
                           }
                        );

                        // start the pump, and accumulate the transaction messages
                        common::message::dispatch::pump( casual::cli::pipe::condition::done( done), handler, in);

                        // we're done in a 'casual-pipe' way
                        cli::pipe::done();

                        // sort/partition on owner. could be a lot of 'owners' (not likely though)
                        algorithm::stable_sort( result, predicate::owner::less);

                        log::line( verbose::log, "result: ", result);

                        return result;
                     }

                     auto finalize( std::vector< cli::message::Transaction>&& transactions)
                     {
                        Trace trace{ "transaction::manager::admin::local::handle::finalize"};

                        if( transactions.empty())
                           return;

                        auto next_range = []( auto range)
                        {
                           return algorithm::sorted::upper_bound( range, *range, handle::predicate::owner::less);
                        };

                        auto handle_finalize = []( auto range)
                        {
                           Trace trace{ "transaction::manager::admin::local::handle::finalize handle_finalize"};
                           cli::message::transaction::Finalize message{ process::handle()};
                           algorithm::copy( range, message.transactions);

                           communication::device::blocking::send( 
                              range::front( message.transactions).trid.owner().ipc, 
                              message);
                        };

                        algorithm::for_each( 
                           range::adapter::make( next_range, range::make( transactions)),
                           handle_finalize); 

                     }
                     
                  } // handle

                  namespace commit
                  {
                     auto option()
                     {
                        auto invoke = []()
                        {
                           Trace trace{ "transaction::manager::admin::local::commit::invoke"};
                           handle::finalize( handle::accumulate());
                        };

                        return argument::Option{
                           std::move( invoke),
                           { "--commit"},
                            R"(sends transaction finalize request to the 'owners' of upstream transactions

* all committable associated transaction will be sent for commit.
* all NOT committable associated transactions will be sent for rollback
* all transaction directives from upstream will be 'terminated', that is, notify the upstream 'owner' and not forward the directive
* all 'forward' 'payloads' downstream will not have any transaction associated

hence, directly downstream there will be no transaction, but users can start new transaction directives downstream.
)"
                        };
                     }

                  } // commit

                  namespace rollback
                  {
                     auto option()
                     {
                        auto invoke = []()
                        {
                           Trace trace{ "transaction::manager::admin::local::rollback::invoke"};

                           auto set_rollback = []( auto& transaction)
                           {
                              transaction.code = decltype( transaction.code)::rollback;
                           };

                           handle::finalize( algorithm::for_each( handle::accumulate(), set_rollback));
                        };

                        return argument::Option{
                           std::move( invoke),
                           { "--rollback"},
                           R"(sends transaction (rollback) finalize request to the 'owners' of upstream transactions

* all associated transaction will be sent for rollback
* all transaction directives from upstream will be 'terminated', that is, notify the upstream 'owner' and not forward the directive
* all 'forward' 'payloads' downstream will not have any transaction associated

hence, directly downstream there will be no transaction, but users can start new transaction directives downstream.
)"
                        };
                     }
                  } // rollback
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
                     local::begin::compound::option(),
                     local::commit::option(),
                     local::rollback::option(),
                     local::scale::instances::option(),
                     local::list::pending::option(),
                     local::information::option(),
                     local::state::option(),
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

