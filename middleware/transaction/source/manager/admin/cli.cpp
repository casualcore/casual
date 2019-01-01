//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/manager/admin/cli.h"
#include "transaction/manager/admin/transactionvo.h"
#include "transaction/manager/admin/server.h"


#include "common/argument.h"
#include "common/environment.h"
#include "common/terminal.h"
#include "common/exception/handle.h"


#include "serviceframework/service/protocol/call.h"
#include "serviceframework/archive/log.h"
#include "serviceframework/archive/create.h"


namespace casual
{
   using namespace common;

   namespace transaction
   {
      namespace manager
      {

         namespace admin
         {

            namespace call
            {

               admin::State state()
               {
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( service::name::state());

                  admin::State result;

                  reply >> CASUAL_MAKE_NVP( result);

                  return result;
               }


               namespace update
               {
                  std::vector< admin::resource::Proxy> instances( const std::vector< admin::update::Instances>& instances)
                  {
                     serviceframework::service::protocol::binary::Call call;

                     call << CASUAL_MAKE_NVP( instances);
                     auto reply = call( service::name::update::instances());

                     std::vector< admin::resource::Proxy> result;

                     reply >> CASUAL_MAKE_NVP( result);

                     return result;
                  }
               } // update

            } // call



            namespace format
            {  
               using time_type = std::chrono::duration< double>;
               
               auto accumulate_statistics( const admin::resource::Proxy& value)
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
                  struct format_global
                  {
                     std::string operator () ( const admin::Transaction& value) const { return value.trid.global; }
                  };

                  struct format_branch
                  {
                     std::string operator () ( const admin::Transaction& value) const { return value.trid.branch; }
                  };

                  struct format_owner
                  {
                     auto operator () ( const admin::Transaction& value) const { return value.trid.owner.pid;}
                  };

                  auto format_state = []( const admin::Transaction& value){
                     return common::string::compose( static_cast< common::code::xa>( value.state));
                  };

                  struct format_resources
                  {
                     std::string operator () ( const admin::Transaction& value)
                     { std::ostringstream out;  out << common::range::make( value.resources); return out.str();}
                  };


                  return common::terminal::format::formatter< admin::Transaction>::construct(
                     common::terminal::format::column( "global", format_global{}, common::terminal::color::yellow),
                     common::terminal::format::column( "branch", format_branch{}, common::terminal::color::grey),
                     common::terminal::format::column( "owner", format_owner{}, common::terminal::color::white, common::terminal::format::Align::right),
                     common::terminal::format::column( "state", format_state, common::terminal::color::green, common::terminal::format::Align::left),
                     common::terminal::format::column( "resources", format_resources{}, common::terminal::color::magenta, common::terminal::format::Align::left)
                  );
               }

               auto resource_proxy()
               {

                  struct format_number_of_instances
                  {
                     std::size_t operator() ( const admin::resource::Proxy& value) const
                     {
                        return value.instances.size();
                     }
                  };

                  auto format_invoked = []( const admin::resource::Proxy& value)
                  {
                     auto result = accumulate_statistics( value);
                     return result.roundtrip.count;
                  };

                  auto format_min = []( const admin::resource::Proxy& value)
                  {
                     auto result = accumulate_statistics( value);
                     return std::chrono::duration_cast< time_type>( result.roundtrip.limit.min).count();
                  };

                  auto format_max = []( const admin::resource::Proxy& value)
                  {
                     auto result = accumulate_statistics( value);
                     return std::chrono::duration_cast< time_type>( result.roundtrip.limit.max).count(); 
                  };

                  auto format_avg = []( const admin::resource::Proxy& value)
                  {
                     auto result = accumulate_statistics( value);
                     if( result.roundtrip.count == 0) return 0.0;
                     return std::chrono::duration_cast< time_type>( result.roundtrip.total / result.roundtrip.count).count();
                  };


                  return common::terminal::format::formatter< admin::resource::Proxy>::construct(
                     common::terminal::format::column( "id", std::mem_fn( &admin::resource::Proxy::id), common::terminal::color::yellow, terminal::format::Align::right),
                     common::terminal::format::column( "key", std::mem_fn( &admin::resource::Proxy::key), common::terminal::color::yellow),
                     common::terminal::format::column( "openinfo", std::mem_fn( &admin::resource::Proxy::openinfo), common::terminal::color::no_color),
                     common::terminal::format::column( "closeinfo", std::mem_fn( &admin::resource::Proxy::closeinfo), common::terminal::color::no_color),
                     terminal::format::column( "invoked", format_invoked, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "min (s)", format_min, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "max (s)", format_max, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "avg (s)", format_avg, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "#", format_number_of_instances{}, terminal::color::white, terminal::format::Align::right)
                  );

               }

               auto resource_instance()
               {
                  auto format_pid = []( const admin::resource::Instance& value) 
                  {
                     return value.process.pid;
                  };

                  auto format_queue = []( const admin::resource::Instance& value) 
                  {
                     return value.process.ipc;
                  };

                  auto format_invoked = []( const admin::resource::Instance& value) 
                  {
                     return value.metrics.roundtrip.count;
                  };

                  auto format_min = []( const admin::resource::Instance& value)
                  {
                     return std::chrono::duration_cast< time_type>( value.metrics.roundtrip.limit.min).count();
                  };

                  auto format_max = []( const admin::resource::Instance& value)
                  {
                     return std::chrono::duration_cast< time_type>( value.metrics.roundtrip.limit.max).count();
                  };

                  auto format_avg = []( const admin::resource::Instance& value)
                  {
                     if( value.metrics.roundtrip.count == 0) return 0.0;
                     return std::chrono::duration_cast< time_type>( value.metrics.roundtrip.total / value.metrics.roundtrip.count).count();
                  };

                  auto format_rm_invoked = []( const admin::resource::Instance& value)
                  {
                     return value.metrics.resource.count;
                  };

                  auto format_rm_min = []( const admin::resource::Instance& value)
                  {
                     return std::chrono::duration_cast< time_type>( value.metrics.resource.limit.min).count();
                  };

                  auto format_rm_max= []( const admin::resource::Instance& value)
                  {
                     return std::chrono::duration_cast< time_type>( value.metrics.resource.limit.max).count();
                  };

                  auto format_rm_avg = []( const admin::resource::Instance& value)
                  {
                     if( value.metrics.resource.count == 0) return 0.0;
                     return std::chrono::duration_cast< time_type>( value.metrics.resource.total / value.metrics.resource.count).count();
                  };


                  return common::terminal::format::formatter< admin::resource::Instance>::construct(
                     terminal::format::column( "id", std::mem_fn( &admin::resource::Instance::id), common::terminal::color::yellow, terminal::format::Align::right),
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
               void list_transactions()
               {
                  auto state = call::state();

                  auto formatter = format::transaction();

                  formatter.print( std::cout, state.transactions);
               }

               void list_pending()
               {
                  auto state = call::state();

                  auto debug = serviceframework::archive::log::writer( std::cout);

                  debug << CASUAL_MAKE_NVP( state.pending.requests);
                  debug << CASUAL_MAKE_NVP( state.persistent.requests);
                  debug << CASUAL_MAKE_NVP( state.persistent.replies);
                  debug << CASUAL_MAKE_NVP( state.log);

               }

               void list_resources()
               {
                  auto state = call::state();

                  auto formatter = format::resource_proxy();

                  formatter.print( std::cout, algorithm::sort( state.resources));
               }

               namespace local
               {
                  namespace
                  {
                     namespace transform
                     {

                        std::vector< admin::resource::Instance> instances( std::vector< admin::resource::Proxy> resources)
                        {
                           std::vector< admin::resource::Instance> result;

                           for( auto& resource : resources)
                           {
                              algorithm::move( resource.instances, result);
                           }
                           return result;
                        }

                     } // transform
                  } // <unnamed>
               } // local

               void list_instances()
               {
                  auto instances = local::transform::instances( call::state().resources);

                  auto formatter = format::resource_instance();

                  formatter.print( std::cout, algorithm::sort( instances));
               }


               void update_instances( const std::vector< std::tuple< common::strong::resource::id::value_type, int>>& values)
               {
                  auto resources = call::update::instances( common::algorithm::transform( values, []( auto& value){
                     admin::update::Instances instance;
                     instance.id = common::strong::resource::id{ std::get< 0>( value)};
                     instance.instances = std::get< 1>( value);
                     return instance;
                  }));

                  auto formatter = format::resource_proxy();

                  formatter.print( std::cout, algorithm::sort( resources));
               }

               void state( const common::optional< std::string>& format)
               {
                  auto state = call::state();
                  auto archive = serviceframework::archive::create::writer::from( format.value_or( ""), std::cout);

                  archive << CASUAL_MAKE_NVP( state);
               }

            } // dispatch

            struct cli::Implementation
            {
               common::argument::Group options()
               {
                  auto complete_state = []( auto values, bool){
                     return std::vector< std::string>{ "json", "yaml", "xml", "ini"};
                  };

                  return common::argument::Group{ [](){}, { "transaction"}, "transaction related administration",
                     common::argument::Option( &dispatch::list_transactions, { "-lt", "--list-transactions" }, "list current transactions"),
                     common::argument::Option( &dispatch::list_resources, { "-lr", "--list-resources" }, "list all resources"),
                     common::argument::Option( &dispatch::list_instances, { "-li", "--list-instances" }, "list resource instances"),
                     common::argument::Option( &dispatch::update_instances, { "-ui", "--update-instances" }, "update instances - -ui [<rm-id> <# instances>]+"),
                     common::argument::Option( &dispatch::list_pending, { "-lp", "--list-pending" }, "list pending tasks"),
                     common::argument::Option( &dispatch::state, complete_state, { "--state" }, "view current state in optional format")
                  };
               }
            };

            cli::cli() = default; 
            cli::~cli() = default; 

            common::argument::Group cli::options() &
            {
               return m_implementation->options();
            }

         } // admin
      } // manager
   } // transaction
} // casual

