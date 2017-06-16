//!
//! casual
//!



#include "common/arguments.h"
#include "common/environment.h"
#include "common/terminal.h"


#include "transaction/manager/admin/transactionvo.h"
#include "transaction/manager/admin/server.h"



#include "sf/service/protocol/call.h"
#include "sf/archive/log.h"
#include "sf/archive/maker.h"


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
                  sf::service::protocol::binary::Call call;
                  auto reply = call( service::name::state());

                  vo::State result;

                  reply >> CASUAL_MAKE_NVP( result);

                  return result;
               }


               namespace update
               {
                  std::vector< vo::resource::Proxy> instances( const std::vector< vo::update::Instances>& instances)
                  {
                     sf::service::protocol::binary::Call call;

                     call << CASUAL_MAKE_NVP( instances);
                     auto reply = call( service::name::update::instances());

                     std::vector< vo::resource::Proxy> result;

                     reply >> CASUAL_MAKE_NVP( result);

                     return result;
                  }
               } // update

            } // call


            namespace global
            {
               bool porcelain = false;

               bool no_color = false;
               bool no_header = false;

            } // global


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

               common::terminal::format::formatter< vo::Transaction> transaction()
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
                     std::string operator () ( const vo::Transaction& value) const { return std::to_string( value.trid.owner.pid); }
                  };

                  struct format_state
                  {
                     std::string operator () ( const vo::Transaction& value)
                     { return common::error::xa::error( value.state);}
                  };

                  struct format_resources
                  {
                     std::string operator () ( const vo::Transaction& value)
                     { std::ostringstream out;  out << common::range::make( value.resources); return out.str();}
                  };


                  return {
                     { global::porcelain, ! global::no_color, ! global::no_header},
                     common::terminal::format::column( "global", format_global{}, common::terminal::color::yellow),
                     common::terminal::format::column( "branch", format_branch{}, common::terminal::color::grey),
                     common::terminal::format::column( "owner", format_owner{}, common::terminal::color::white, common::terminal::format::Align::right),
                     common::terminal::format::column( "state", format_state{}, common::terminal::color::green, common::terminal::format::Align::left),
                     common::terminal::format::column( "resources", format_resources{}, common::terminal::color::magenta, common::terminal::format::Align::left)
                  };
               }

               common::terminal::format::formatter< vo::resource::Proxy> resource_proxy()
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


                  return {
                     { global::porcelain, ! global::no_color, ! global::no_header},
                     common::terminal::format::column( "id", std::mem_fn( &vo::resource::Proxy::id), common::terminal::color::yellow, terminal::format::Align::right),
                     common::terminal::format::column( "key", std::mem_fn( &vo::resource::Proxy::key), common::terminal::color::yellow),
                     common::terminal::format::column( "openinfo", std::mem_fn( &vo::resource::Proxy::openinfo), common::terminal::color::no_color),
                     common::terminal::format::column( "closeinfo", std::mem_fn( &vo::resource::Proxy::closeinfo), common::terminal::color::no_color),
                     terminal::format::column( "invoked", format_invoked{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "min (us)", format_min{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "max (us)", format_max{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "avg (us)", format_avg{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "#", format_number_of_instances{}, terminal::color::white, terminal::format::Align::right),
                  };

               }

               common::terminal::format::formatter< vo::resource::Instance> resource_instance()
               {
                  struct format_pid
                  {
                     platform::pid::type operator() ( const vo::resource::Instance& value) const
                     {
                        return value.process.pid;
                     }
                  };

                  struct format_queue
                  {
                     platform::pid::type operator() ( const vo::resource::Instance& value) const
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

                  return {
                     { global::porcelain, ! global::no_color, ! global::no_header},
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
                     terminal::format::column( "rm-avg (us)", format_rm_avg{}, terminal::color::blue, terminal::format::Align::right),
                  };

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

                  sf::archive::log::Writer debug{ std::cout};

                  debug << CASUAL_MAKE_NVP( state.pending.requests);
                  debug << CASUAL_MAKE_NVP( state.persistent.requests);
                  debug << CASUAL_MAKE_NVP( state.persistent.replies);
                  debug << CASUAL_MAKE_NVP( state.log);

               }

               void list_resources()
               {
                  auto state = call::state();

                  auto formatter = format::resource_proxy();

                  formatter.print( std::cout, range::sort( state.resources));
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
                              range::move( resource.instances, result);
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

                  formatter.print( std::cout, range::sort( instances));
               }

               namespace local
               {
                  namespace
                  {
                     namespace transform
                     {
                        std::vector< vo::update::Instances> instances( const std::vector< std::size_t>& values)
                        {
                           if( values.size() % 2 != 0)
                           {
                              throw exception::invalid::Argument{ "use: --update-instances [<rm-id> <# instances>]+"};
                           }

                           std::vector< vo::update::Instances> result;

                           auto current = std::begin( values);

                           while( current != std::end( values))
                           {
                              vo::update::Instances instance;
                              instance.id = *current;
                              instance.instances = *( current + 1);

                              result.push_back( std::move( instance));
                              current += 2;
                           }
                           return result;
                        }

                     } // transform
                  } // <unnamed>
               } // local

               void update_instances( const std::vector< std::size_t>& values)
               {
                  auto resources = call::update::instances( local::transform::instances( values));

                  auto formatter = format::resource_proxy();

                  formatter.print( std::cout, range::sort( resources));
               }

               void state( const std::vector< std::string>& values)
               {
                  auto state = call::state();

                  if( values.empty())
                  {
                     sf::archive::log::Writer archive( std::cout);
                     archive << CASUAL_MAKE_NVP( state);
                  }
                  else
                  {
                     auto archive = sf::archive::writer::from::name( std::cout, values.front());

                     archive << CASUAL_MAKE_NVP( state);

                  }
               }

            } // dispatch


            int main( int argc, char **argv)
            {

               common::Arguments arguments{
               {
                  common::argument::directive( {"--no-header"}, "do not print headers", global::no_header),
                  common::argument::directive( {"--no-color"}, "do not use color", global::no_color),
                  common::argument::directive( {"--porcelain"}, "Easy to parse format", global::porcelain),
                  common::argument::directive( { "-lt", "--list-transactions" }, "list current transactions", &dispatch::list_transactions),
                  common::argument::directive( { "-lr", "--list-resources" }, "list current transactions", &dispatch::list_resources),
                  common::argument::directive( { "-li", "--list-instances" }, "list current transactions", &dispatch::list_instances),
                  common::argument::directive( { "-ui", "--update-instances" }, "update instances - -ui [<rm-id> <# instances>]+", &dispatch::update_instances),
                  common::argument::directive( { "-lp", "--list-pending" }, "list pending tasks", &dispatch::list_pending),
                  common::argument::directive( common::argument::cardinality::Any{}, { "--state" }, "view current state in supplied format: --state (yaml|json|xml|ini)", &dispatch::state)
               }};


               try
               {
                  arguments.parse( argc, argv);

               }
               catch( const std::exception& exception)
               {
                  std::cerr << "error: " << exception.what() << std::endl;
                  return 10;
               }
               return 0;
            }

         } // admin
      } // manager
   } // transaction
} // casual

int main( int argc, char **argv)
{
   return casual::transaction::manager::admin::main( argc, argv);
}

