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
#include "common/exception/handle.h"
#include "common/serialize/create.h"

#include "serviceframework/service/protocol/call.h"


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

                     admin::model::State state()
                     {
                        serviceframework::service::protocol::binary::Call call;
                        auto reply = call( service::name::state());

                        admin::model::State result;

                        reply >> CASUAL_NAMED_VALUE( result);

                        return result;
                     }


                     namespace scale
                     {
                        std::vector< admin::model::resource::Proxy> instances( const std::vector< admin::model::scale::Instances>& instances)
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

                        auto format_branch = []( auto& value) { return value.branches.size();};

                        auto format_owner = []( auto& value){ return value.global.owner.pid;};

                        auto format_state = []( auto& value){
                           return common::string::compose( static_cast< common::code::xa>( value.state));
                        };

                        auto format_resources = []( auto& value)
                        {
                           std::vector< model::resource::id_type> resources;
                           
                           for( auto& branch : value.branches)
                           {
                              algorithm::append( branch.resources, resources);
                           }
                           return common::string::compose( algorithm::unique( algorithm::sort( resources)));
                        };

                        return common::terminal::format::formatter< admin::model::Transaction>::construct(
                           common::terminal::format::column( "global", format_global, common::terminal::color::yellow),
                           common::terminal::format::column( "branch", format_branch, common::terminal::color::grey),
                           common::terminal::format::column( "owner", format_owner, common::terminal::color::white, common::terminal::format::Align::right),
                           common::terminal::format::column( "state", format_state, common::terminal::color::green, common::terminal::format::Align::left),
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

                     auto resource_instance()
                     {
                        auto format_pid = []( const admin::model::resource::Instance& value) 
                        {
                           return value.process.pid;
                        };

                        auto format_queue = []( const admin::model::resource::Instance& value) 
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
                           terminal::format::column( "queue", format_queue, common::terminal::color::no_color, terminal::format::Align::right),
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

                  namespace dispatch
                  {
                     namespace list
                     {
                        namespace transaction
                        {
                           void invoke()
                           {
                              auto state = call::state();
                              format::transaction().print( std::cout, state.transactions);
                           }

                           constexpr auto description = R"(list current transactions)";

                        } // transaction

                        namespace resources
                        {
                           void invoke()
                           {
                              auto state = call::state();
                              format::resource_proxy().print( std::cout, algorithm::sort( state.resources));
                           }

                           constexpr auto description = R"(list all resources)";
                        } // resources
                        
                        namespace instances
                        {
                           void invoke()
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
                           }

                           constexpr auto description = R"(list resource instances)";
                        } // instances
                        
                        namespace pending
                        {
                           void invoke()
                           {
                              auto state = call::state();

                              auto debug = common::serialize::log::writer();

                              debug << CASUAL_NAMED_VALUE( state.pending);
                              debug << CASUAL_NAMED_VALUE( state.log);

                              debug.consume( std::cout);
                           }

                           constexpr auto description = R"(list pending tasks)";
                           
                        } // pending
                     } // list

                     namespace scale
                     {
                        namespace instances
                        {
                           void invoke( std::vector< std::tuple< std::string, int>> values)
                           {
                              auto resources = call::scale::instances( common::algorithm::transform( values, []( auto& value){
                                 admin::model::scale::Instances instance;
                                 instance.name = std::get< 0>( value);
                                 instance.instances = std::get< 1>( value);
                                 return instance;
                              }));

                              format::resource_proxy().print( std::cout, algorithm::sort( resources));
                           }

                           constexpr auto description = R"(scale resource proxy instances)";

                           auto complete = []( auto values, bool help) -> std::vector< std::string>
                           {
                              if( help)
                                 return { "rm-id", "# instances"};

                              if( values.size() % 2 == 0)
                                 return algorithm::transform( call::state().resources, []( auto& r){ return r.name;});
         
                              return { common::argument::reserved::name::suggestions::value()}; 
                           };
                        } // instances
                     } // scale


                     void state( const common::optional< std::string>& format)
                     {
                        auto state = call::state();
                        auto archive = common::serialize::create::writer::from( format.value_or( ""));
                        archive << CASUAL_NAMED_VALUE( state);
                        archive.consume( std::cout);
                     }

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
                              { "transaction.manager.resource.metrics.resource.avarage", 
                                 string::compose( average( metric_resource_total( state.resources), metric_resource_count( state.resources)))},
                              { "transaction.manager.resource.metrics.roundtrip.count", string::compose( metric_roundtrip_count( state.resources))},
                              { "transaction.manager.resource.metrics.roundtrip.total", string::compose( metric_roundtrip_total( state.resources))},
                              { "transaction.manager.resource.metrics.roundtrip.avarage", 
                                 string::compose( average( metric_roundtrip_total( state.resources), metric_roundtrip_count( state.resources)))},

                              { "transaction.manager.log.writes", string::compose( state.log.writes)},
                              { "transaction.manager.log.update.prepare", string::compose( state.log.update.prepare)},
                              { "transaction.manager.log.update.remove", string::compose( state.log.update.remove)},

                              

                           };
                        } 
                        
                        void invoke()
                        {
                           terminal::formatter::key::value().print( std::cout, call());
                        }

                        constexpr auto description = R"(collect aggregated information about transactions in this domain)";

                     } // information

                  } // dispatch

                  namespace complete
                  {
                     auto state = []( auto values, bool)
                     {
                        return std::vector< std::string>{ "json", "yaml", "xml", "ini"};
                     };


                  } // complete

               } // <unnamed>
            } // local

            struct cli::Implementation
            {
               argument::Group options()
               {
                  return argument::Group{ [](){}, { "transaction"}, "transaction related administration",
                     argument::Option( &local::dispatch::list::transaction::invoke, { "-lt", "--list-transactions" }, local::dispatch::list::transaction::description),
                     argument::Option( &local::dispatch::list::resources::invoke, { "-lr", "--list-resources" }, local::dispatch::list::resources::description),
                     argument::Option( &local::dispatch::list::instances::invoke, { "-li", "--list-instances" }, local::dispatch::list::instances::description),
                     argument::Option( 
                        argument::option::one::many( &local::dispatch::scale::instances::invoke), 
                        local::dispatch::scale::instances::complete, 
                        { "-si", "--scale-instances" }, 
                        local::dispatch::scale::instances::description),
                     argument::Option( &local::dispatch::list::pending::invoke, { "-lp", "--list-pending" }, local::dispatch::list::pending::description),
                     argument::Option( &local::dispatch::information::invoke, { "--information" }, local::dispatch::information::description),
                     argument::Option( &local::dispatch::state, local::complete::state, { "--state" }, "view current state in optional format")
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
               return local::dispatch::information::call();
            }

         } // admin
      } // manager
   } // transaction
} // casual

