//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/manager/admin/cli.h"
#include "gateway/manager/admin/model.h"
#include "gateway/manager/admin/server.h"

#include "common/argument.h"
#include "common/terminal.h"
#include "common/chronology.h"
#include "common/transcode.h"
#include "common/exception/handle.h"
#include "common/event/listen.h"

#include "serviceframework/service/protocol/call.h"
#include "common/serialize/create.h"
#include "serviceframework/log.h"

#include "xatmi.h"

#include <iostream>

namespace casual
{
   using namespace common;

   namespace gateway::manager::admin
   {
      namespace local
      {
         namespace
         {
            
            namespace call
            {
               manager::admin::model::State state()
               {
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( manager::admin::service::name::state);

                  manager::admin::model::State result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }

               auto rediscover()
               {
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( manager::admin::service::name::rediscover);

                  Uuid result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }

            } // call

            namespace format
            {
               std::string_view dash_if_empty( const std::string& string)
               {
                  if( string.empty())
                     return "-";
                  return string;
               }

               auto created = []( auto& value) -> std::string
               {
                  if( value.created != platform::time::point::limit::zero())
                     return chronology::utc::offset( value.created);

                  return "-";
               };

               auto group = []( auto& value) -> decltype( value.group)
               {
                  return value.group;
               };

               auto alias = []( auto& value) -> decltype( value.alias)
               {
                  return value.alias;
               };

               auto connections()
               {
                  auto format_domain_name = []( auto& value)
                  { 
                     return dash_if_empty( value.remote.name);
                  };
                  auto format_domain_id = []( auto& value) -> std::string
                  {
                     if( value.remote.id) 
                        return transcode::hex::encode( value.remote.id.get());
                     return "-";
                  };



                  auto format_bound = []( auto& value)
                  {
                     switch( value.bound)
                     {
                        using Enum = decltype( value.bound);
                        case Enum::in: return "in";
                        case Enum::out: return "out";
                        case Enum::unknown: return "unknown";
                     }
                     return "<unknown>";
                  };


                  auto format_local_address = []( auto& value)
                  {
                     return dash_if_empty( value.address.local);
                  };

                  auto format_peer_address = []( auto& value)
                  {
                     return dash_if_empty( value.address.peer);
                  };

                  using Formatter = terminal::format::formatter<  manager::admin::model::Connection>;

                  if( ! terminal::output::directive().porcelain())
                  {
                     return Formatter::construct( 
                        terminal::format::column( "name", format_domain_name, terminal::color::yellow),
                        terminal::format::column( "id", format_domain_id, terminal::color::no_color),
                        terminal::format::column( "group", format::group, terminal::color::yellow),
                        terminal::format::column( "bound", format_bound, terminal::color::magenta),
                        terminal::format::column( "local", format_local_address, terminal::color::white),
                        terminal::format::column( "peer", format_peer_address, terminal::color::white),
                        terminal::format::column( "created", format::created, terminal::color::blue)
                     );
                  }
                  else
                  {
                     //! @deprecated remove in 2.0 - the whole else branch...

                     auto format_runlevel = []( auto& value)
                     {
                        switch( value.runlevel())
                        {
                           using Enum = decltype( value.runlevel());
                           case Enum::connecting: return "connecting";
                           case Enum::connected: return "online";
                        }
                        return "<unknown>";
                     };

                     auto format_pid = []( auto& value){ return value.process.pid;};
                     auto format_ipc = []( auto& value){ return value.process.ipc;};

                     
                     return Formatter::construct( 
                        terminal::format::column( "name", format_domain_name),
                        terminal::format::column( "id", format_domain_id),
                        terminal::format::column( "bound", format_bound),
                        terminal::format::column( "pid", format_pid),
                        terminal::format::column( "ipc", format_ipc),
                        terminal::format::column( "runlevel", format_runlevel),
                        terminal::format::column( "local", format_local_address),
                        terminal::format::column( "peer", format_peer_address),
                        terminal::format::column( "group", format::group),
                        terminal::format::column( "created", format::created)
                     );
                  }
               }

               auto listeners() 
               {
                  using Formatter = terminal::format::formatter<  manager::admin::model::Listener>;

                  if( ! terminal::output::directive().porcelain())
                  {
                     auto format_address = []( auto& listener)
                     { 
                        return string::compose( listener.address.host, ':', listener.address.port);
                     };

                     return Formatter::construct( 
                        terminal::format::column( "group", format::group, terminal::color::yellow),
                        terminal::format::column( "address", format_address, terminal::color::white),
                        terminal::format::column( "created", format::created, terminal::color::blue)
                     );
                  }
                  else
                  {
                     //! @deprecated remove in 2.0 - the whole else branch...

                     auto format_host = []( auto& listener){ return listener.address.host;};
                     auto format_port = []( auto& listener){ return listener.address.port;};

                     auto format_limit_size = []( auto& listener) -> std::string
                     { 
                        if( listener.limit.size)
                           return std::to_string( listener.limit.size);
                        return "-";
                     };

                     auto format_limit_messages = []( auto& listener) -> std::string
                     { 
                        if( listener.limit.messages)
                           return std::to_string( listener.limit.messages);
                        return "-";
                     };

                     return Formatter::construct( 
                        terminal::format::column( "host", format_host),
                        terminal::format::column( "port", format_port),
                        terminal::format::column( "limit size", format_limit_size),
                        terminal::format::column( "limit messages", format_limit_messages),
                        terminal::format::column( "group", format::group),
                        terminal::format::column( "created", format::created)
                     );
                  }
               }
               

               namespace groups
               {
                  auto format_runlevel = []( auto& value)
                  {
                     return value.runlevel;
                  };

                  auto format_pid = []( auto& value)
                  {
                     return value.process.pid;
                  };

                  auto format_connect = []( auto& value)
                  {
                     return value.connect;
                  };

                  auto inbound()
                  {
                     auto format_limit = []( auto& value) -> std::string
                     { 
                        if( value.limit.size > 0 && value.limit.messages > 0)
                           return string::compose( value.limit.size, ':', value.limit.messages);
                        return "-";
                     };

                     using Formatter = terminal::format::formatter< manager::admin::model::inbound::Group>;

                     return Formatter::construct( 
                        terminal::format::column( "alias", format::alias, terminal::color::yellow),
                        terminal::format::column( "pid", format_pid, terminal::color::white),
                        terminal::format::column( "runlevel", format_runlevel, terminal::color::no_color),
                        terminal::format::column( "connect", format_connect, terminal::color::magenta),
                        terminal::format::column( "limit", format_limit, terminal::color::cyan)
                     );
                  }

                  auto outbound()
                  {
                     auto format_order = []( auto& value)
                     { 
                        return value.order;
                     };

                     using Formatter = terminal::format::formatter< manager::admin::model::outbound::Group>;

                     return Formatter::construct( 
                        terminal::format::column( "alias", format::alias, terminal::color::yellow),
                        terminal::format::column( "pid", format_pid, terminal::color::white),
                        terminal::format::column( "runlevel", format_runlevel, terminal::color::no_color),
                        terminal::format::column( "connect", format_connect, terminal::color::magenta),
                        terminal::format::column( "order", format_order, terminal::color::cyan)
                     );
                  }
               } // groups

            } // format

            namespace option
            {
               namespace list
               {
                  auto connections()
                  {
                     auto invoke = []()
                     {
                        format::connections().print( std::cout, call::state().connections);
                     };

                     return argument::Option{ 
                        std::move( invoke), 
                        { "-c", "--list-connections"}, 
                        "list all connections"};
                  }

                  auto listeners()
                  {
                     auto invoke = []()
                     {
                        format::listeners().print( std::cout, call::state().listeners);
                     };

                     return argument::Option{ 
                        std::move( invoke), 
                        { "-l", "--list-listeners"}, 
                        "list all listeners"};
                  }

                  namespace groups
                  {
                     auto inbound()
                     {
                        auto invoke = []()
                        {
                           format::groups::inbound().print( std::cout, call::state().inbound.groups);
                        };

                        return argument::Option{ 
                           std::move( invoke), 
                           { "--list-inbound-groups"}, 
                           "list all inbound groups"};
                     }

                     auto outbound()
                     {
                        auto invoke = []()
                        {
                           format::groups::outbound().print( std::cout, call::state().outbound.groups);
                        };

                        return argument::Option{ 
                           std::move( invoke), 
                           { "--list-outbound-groups"}, 
                           "list all outbound groups"};
                     }
                  } // groups
               } // list
 
               auto rediscover()
               {
                  auto invoke = []()
                  {
                     if( ! terminal::output::directive().block())
                     {
                        call::rediscover();
                        return;
                     }
                     
                     Uuid correlation;

                     auto condition = event::condition::compose( 
                        event::condition::prelude( [&correlation]() { correlation = call::rediscover();}),
                        event::condition::done( [&correlation](){ return correlation.empty();}));
                     
                     event::listen( condition, 
                        [&correlation]( const common::message::event::Task& task)
                        {
                           common::message::event::terminal::print( std::cout, task);

                           if( task.done())
                              correlation = {};
                        },
                        []( const common::message::event::sub::Task& task)
                        {
                           common::message::event::terminal::print( std::cout, task);
                        }
                     );
                  };

                  return argument::Option{ 
                     std::move( invoke), 
                     { "--rediscover"}, 
                     R"(rediscover all outbound connections)"
                  };

               }
               
               auto state()
               {
                  auto invoke = []( const std::optional< std::string>& format)
                  {
                     auto state = call::state();

                     auto archive = common::serialize::create::writer::from( format.value_or( ""));
                     archive << CASUAL_NAMED_VALUE( state);
                     archive.consume( std::cout);
                  };

                  auto complete = []( auto values, bool) -> std::vector< std::string>
                  {
                     return { "json", "yaml", "xml", "ini"};
                  };

                  return argument::Option{ 
                     std::move( invoke),
                     std::move( complete),
                     {"--state"}, 
                     "gateway state"};
               }

            } // option
         } // <unnamed>
      } // local



      struct cli::Implementation
      {
         common::argument::Group options()
         {
            return { [](){}, { "gateway"}, "gateway related administration",
               local::option::list::connections(),
               local::option::list::listeners(),
               local::option::list::groups::inbound(),
               local::option::list::groups::outbound(),
               local::option::rediscover(),
               local::option::state()
            };
         }
      };

      cli::cli() = default; 
      cli::~cli() = default; 

      common::argument::Group cli::options() &
      {
         return m_implementation->options();
      }
            
 
   } // gateway::manager::admin
} // casual




