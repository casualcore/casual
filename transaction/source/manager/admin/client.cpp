//!
//! client.cpp
//!
//! Created on: Jun 14, 2015
//!     Author: Lazan
//!



#include "common/arguments.h"
#include "common/environment.h"
#include "common/terminal.h"


#include "transaction/manager/admin/transactionvo.h"



#include "sf/xatmi_call.h"
#include "sf/archive/log.h"


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
                  sf::xatmi::service::binary::Sync service( ".casual.transaction.state");
                  auto reply = service();

                  vo::State serviceReply;

                  reply >> CASUAL_MAKE_NVP( serviceReply);

                  return serviceReply;
               }


               namespace update
               {
                  std::vector< vo::resource::Proxy> instances( const std::vector< vo::update::Instances>& instances)
                  {
                     sf::xatmi::service::binary::Sync service( ".casual.transaction.update.instances");

                     service << CASUAL_MAKE_NVP( instances);

                     auto reply = service();

                     std::vector< vo::resource::Proxy> serviceReply;

                     reply >> CASUAL_MAKE_NVP( serviceReply);

                     return serviceReply;
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
                  struct format_state
                  {

                     std::size_t width( const vo::resource::Proxy& value) const
                     {
                        return value.instances.size();
                     }

                     void print( std::ostream& out, const vo::resource::Proxy& value, std::size_t width, bool color) const
                     {
                        if( color)
                        {
                           for( auto& instance : value.instances)
                           {
                              switch( instance.state)
                              {
                                 case vo::resource::Instance::State::absent:
                                 case vo::resource::Instance::State::started: out << terminal::color::cyan.start() << '^'; break;
                                 case vo::resource::Instance::State::idle: out << terminal::color::green.start() << '+'; break;
                                 case vo::resource::Instance::State::busy: out << terminal::color::yellow.start() << '*'; break;
                                 case vo::resource::Instance::State::shutdown: out << terminal::color::red.start() << 'x'; break;
                                 default: out << terminal::color::red.start() <<  '-'; break;
                              }
                           }
                           out << terminal::color::green.end();
                        }
                        else
                        {
                           for( auto& instance : value.instances)
                           {
                              switch( instance.state)
                              {
                                 case vo::resource::Instance::State::absent:
                                 case vo::resource::Instance::State::started: out << '^'; break;
                                 case vo::resource::Instance::State::idle: out << '+'; break;
                                 case vo::resource::Instance::State::busy: out << '*'; break;
                                 case vo::resource::Instance::State::shutdown: out << 'x'; break;
                                 default: out << '-'; break;
                              }
                           }
                        }

                        //
                        // Pad manually
                        //

                        if( width != 0 )
                        {
                           out << std::string( width - value.instances.size(), ' ');
                        }
                     }
                  };

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
                        auto result = value.invoked;

                        for( auto& instance : value.instances)
                        {
                           result += instance.invoked;
                        }
                        return result;
                     }
                  };


                  return {
                     { global::porcelain, ! global::no_color, ! global::no_header},
                     common::terminal::format::column( "id", std::mem_fn( &vo::resource::Proxy::id), common::terminal::color::yellow, terminal::format::Align::right),
                     common::terminal::format::column( "key", std::mem_fn( &vo::resource::Proxy::key), common::terminal::color::yellow),
                     common::terminal::format::column( "openinfo", std::mem_fn( &vo::resource::Proxy::openinfo), common::terminal::color::no_color),
                     common::terminal::format::column( "closeinfo", std::mem_fn( &vo::resource::Proxy::closeinfo), common::terminal::color::no_color),
                     terminal::format::column( "invoked", format_invoked{}, terminal::color::blue, terminal::format::Align::right),
                     terminal::format::column( "#", format_number_of_instances{}, terminal::color::white, terminal::format::Align::right),
                     terminal::format::custom_column( "state", format_state{}),

                  };

               }

               common::terminal::format::formatter< vo::resource::Instance> resource_instance()
               {
                  struct format_pid
                  {
                     platform::pid_type operator() ( const vo::resource::Instance& value) const
                     {
                        return value.process.pid;
                     }
                  };

                  struct format_queue
                  {
                     platform::pid_type operator() ( const vo::resource::Instance& value) const
                     {
                        return value.process.queue;
                     }
                  };

                  struct format_state
                  {
                     using State = vo::resource::Instance::State;

                     std::size_t width( const vo::resource::Instance& value) const
                     {
                        switch( value.state)
                        {
                           case State::absent: return 6;
                           case State::shutdown: return 8;
                           default: return 4;
                        }
                     }

                     void print( std::ostream& out, const vo::resource::Instance& value, std::size_t width, bool color) const
                     {
                        out << std::setfill( ' ');

                        if( color)
                        {
                           switch( value.state)
                           {
                              case State::absent: out << std::right << std::setw( width) << terminal::color::red << "absent"; break;
                              case State::idle: out << std::right << std::setw( width) << terminal::color::green << "idle"; break;
                              case State::busy: out << std::right << std::setw( width) << terminal::color::yellow << "busy"; break;
                              case State::shutdown: out << std::right << std::setw( width) << terminal::color::red << "shutdown"; break;
                              default: out << std::right << std::setw( width) << terminal::color::red << "error"; break;
                           }
                        }
                        else
                        {
                           switch( value.state)
                           {
                              case State::absent: out << std::right << std::setw( width)  << "absent"; break;
                              case State::idle: out << std::right << std::setw( width) << "idle"; break;
                              case State::busy: out << std::right << std::setw( width) << "busy"; break;
                              case State::shutdown: out << std::right << std::setw( width) << "shutdown"; break;
                              default: out << std::right << std::setw( width) << "error"; break;
                           }
                        }
                     }
                  };

                  return {
                     { global::porcelain, ! global::no_color, ! global::no_header},
                     common::terminal::format::column( "id", std::mem_fn( &vo::resource::Instance::id), common::terminal::color::yellow, terminal::format::Align::right),
                     common::terminal::format::column( "pid", format_pid{}, common::terminal::color::white, terminal::format::Align::right),
                     common::terminal::format::column( "queue", format_queue{}, common::terminal::color::no_color, terminal::format::Align::right),
                     common::terminal::format::column( "invoked", std::mem_fn( &vo::resource::Instance::invoked), common::terminal::color::blue, terminal::format::Align::right),
                     common::terminal::format::custom_column( "state", format_state{}),
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

            } // dispatch


            int main( int argc, char **argv)
            {
               common::Arguments arguments{ R"(
   usage: 
     
   )"};

               arguments.add(
                  common::argument::directive( {"--no-header"}, "do not print headers", global::no_header),
                  common::argument::directive( {"--no-color"}, "do not use color", global::no_color),
                  common::argument::directive( {"--porcelain"}, "Easy to parse format", global::porcelain),
                  common::argument::directive( { "-lt", "--list-transactions" }, "list current transactions", &dispatch::list_transactions),
                  common::argument::directive( { "-lr", "--list-resources" }, "list current transactions", &dispatch::list_resources),
                  common::argument::directive( { "-li", "--list-instances" }, "list current transactions", &dispatch::list_instances),
                  common::argument::directive( { "-ui", "--update-instances" }, "update instances - -ui [<rm-id> <# instances>]+", &dispatch::update_instances),
                  common::argument::directive( { "-lp", "--list-pending" }, "list pending tasks", &dispatch::list_pending)
                  );


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

