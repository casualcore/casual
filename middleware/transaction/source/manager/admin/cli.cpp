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

               vo::State state()
               {
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( service::name::state());

                  vo::State result;

                  reply >> CASUAL_MAKE_NVP( result);

                  return result;
               }


               namespace update
               {
                  std::vector< vo::resource::Proxy> instances( const std::vector< vo::update::Instances>& instances)
                  {
                     serviceframework::service::protocol::binary::Call call;

                     call << CASUAL_MAKE_NVP( instances);
                     auto reply = call( service::name::update::instances());

                     std::vector< vo::resource::Proxy> result;

                     reply >> CASUAL_MAKE_NVP( result);

                     return result;
                  }
               } // update

            } // call


            namespace format
            {

               vo::Stats accumulate_statistics( const vo::resource::Proxy& value)
               {
                  auto result = value.statistics;

                  for( auto& instance : value.instances)
                  {
                     result += instance.statistics;
                  }
                  return result;
               }

               auto transaction()
               {
                  struct format_global
                  {
                     std::string operator () ( const vo::Transaction& value) const { return value.trid.global; }
                  };

                  struct format_branch
                  {
                     std::string operator () ( const vo::Transaction& value) const { return value.trid.branch; }
                  };

                  struct format_owner
                  {
                     auto operator () ( const vo::Transaction& value) const { return value.trid.owner.pid;}
                  };

                  auto format_state = []( const vo::Transaction& value){
                     return common::string::compose( static_cast< common::code::xa>( value.state));
                  };

                  struct format_resources
                  {
                     std::string operator () ( const vo::Transaction& value)
                     { std::ostringstream out;  out << common::range::make( value.resources); return out.str();}
                  };


                  return common::terminal::format::formatter< vo::Transaction>::construct(
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
                     std::size_t operator() ( const vo::resource::Proxy& value) const
                     {
                        return value.instances.size();
                     }
                  };


                  struct format_invoked
                  {
                     std::size_t operator() ( const vo::resource::Proxy& value) const
                     {
                        auto result = accumulate_statistics( value);
                        return result.roundtrip.invoked;
                     }
                  };



                  struct format_min
                  {
                     std::size_t operator() ( const vo::resource::Proxy& value) const
                     {
                        auto result = accumulate_statistics( value);
                        if( result.roundtrip.invoked == 0) return 0;
                        return result.roundtrip.min.count();
                     }
                  };

                  struct format_max
                  {
                     std::size_t operator() ( const vo::resource::Proxy& value) const
                     {
                        auto result = accumulate_statistics( value);
                        if( result.roundtrip.invoked == 0) return 0;
                        return result.roundtrip.max.count();
                     }
                  };

                  struct format_avg
                  {
                     std::size_t operator() ( const vo::resource::Proxy& value) const
                     {
                        auto result = accumulate_statistics( value);

                        if( result.roundtrip.invoked == 0) return 0;
                        return ( result.roundtrip.total / result.roundtrip.invoked).count();
                     }
                  };


                  return common::terminal::format::formatter< vo::resource::Proxy>::construct(
                     common::terminal::format::column( "id", std::mem_fn( &vo::resource::Proxy::id), common::terminal::color::yellow, terminal::format::Align::right),
                     common::terminal::format::column( "key", std::mem_fn( &vo::resource::Proxy::key), common::terminal::color::yellow),
                     common::terminal::format::column( "openinfo", std::mem_fn( &vo::resource::Proxy::openinfo), common::terminal::color::no_color),
                     common::terminal::format::column( "closeinfo", std::mem_fn( &vo::resource::Proxy::closeinfo), common::terminal::color::no_color),
                     terminal::format::column( "invoked", format_invoked{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "min (us)", format_min{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "max (us)", format_max{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "avg (us)", format_avg{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "#", format_number_of_instances{}, terminal::color::white, terminal::format::Align::right)
                  );

               }

               auto resource_instance()
               {
                  struct format_pid
                  {
                     strong::process::id operator() ( const vo::resource::Instance& value) const
                     {
                        return value.process.pid;
                     }
                  };

                  struct format_queue
                  {
                     auto operator() ( const vo::resource::Instance& value) const
                     {
                        return value.process.queue;
                     }
                  };


                  struct format_invoked
                  {
                     std::size_t operator() ( const vo::resource::Instance& value) const
                     {
                        return value.statistics.roundtrip.invoked;
                     }
                  };

                  struct format_min
                  {
                     std::size_t operator() ( const vo::resource::Instance& value) const
                     {
                        if( value.statistics.roundtrip.invoked == 0) return 0;
                        return value.statistics.roundtrip.min.count();
                     }
                  };

                  struct format_max
                  {
                     std::size_t operator() ( const vo::resource::Instance& value) const
                     {
                        if( value.statistics.roundtrip.invoked == 0) return 0;
                        return value.statistics.roundtrip.max.count();
                     }
                  };

                  struct format_avg
                  {
                     std::size_t operator() ( const vo::resource::Instance& value) const
                     {
                        if( value.statistics.roundtrip.invoked == 0) return 0;
                        return ( value.statistics.roundtrip.total / value.statistics.roundtrip.invoked).count();
                     }
                  };

                  struct format_rm_invoked
                  {
                     std::size_t operator() ( const vo::resource::Instance& value) const
                     {
                        return value.statistics.resource.invoked;
                     }
                  };

                  struct format_rm_min
                  {
                     std::size_t operator() ( const vo::resource::Instance& value) const
                     {
                        if( value.statistics.resource.invoked == 0) return 0;
                        return value.statistics.resource.min.count();
                     }
                  };

                  struct format_rm_max
                  {
                     std::size_t operator() ( const vo::resource::Instance& value) const
                     {
                        if( value.statistics.resource.invoked == 0) return 0;
                        return value.statistics.resource.max.count();
                     }
                  };

                  struct format_rm_avg
                  {
                     std::size_t operator() ( const vo::resource::Instance& value) const
                     {
                        if( value.statistics.resource.invoked == 0) return 0;
                        return ( value.statistics.resource.total / value.statistics.resource.invoked).count();
                     }
                  };

                  return common::terminal::format::formatter< vo::resource::Instance>::construct(
                     terminal::format::column( "id", std::mem_fn( &vo::resource::Instance::id), common::terminal::color::yellow, terminal::format::Align::right),
                     terminal::format::column( "pid", format_pid{}, common::terminal::color::white, terminal::format::Align::right),
                     terminal::format::column( "queue", format_queue{}, common::terminal::color::no_color, terminal::format::Align::right),
                     terminal::format::column( "invoked", format_invoked{}, common::terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "min (us)", format_min{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "max (us)", format_max{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "avg (us)", format_avg{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rm-invoked", format_rm_invoked{}, common::terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rm-min (us)", format_rm_min{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rm-max (us)", format_rm_max{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "rm-avg (us)", format_rm_avg{}, terminal::color::blue, terminal::format::Align::right)
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

                        std::vector< vo::resource::Instance> instances( std::vector< vo::resource::Proxy> resources)
                        {
                           std::vector< vo::resource::Instance> result;

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
                     vo::update::Instances instance;
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

