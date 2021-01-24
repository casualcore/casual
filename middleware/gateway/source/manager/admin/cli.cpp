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

   namespace gateway
   {
      namespace normalize
      {
         std::string timestamp( const platform::time::point::type& time)
         {
            if( time != platform::time::point::limit::zero())
            {
               return chronology::utc::offset( time);
            }
            return "-";
         }
      } // normalize

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
         std::string dash_if_empty( std::string s)
         {
            if( s.empty())
               return "-";
            return s;
         }

         auto connections()
         {
            auto format_domain_name = []( auto& value) -> std::string
            { 
               return dash_if_empty( value.remote.name);
            };
            auto format_domain_id = []( auto& value) -> std::string
            {
               if( value.remote.id) 
                  return transcode::hex::encode( value.remote.id.get());
               return "-";
            };

            auto format_pid = []( auto& value){ return value.process.pid;};
            auto format_ipc = []( auto& value){ return value.process.ipc;};

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

            auto format_runlevel = []( auto& value)
            {
               switch( value.runlevel)
               {
                  using Enum = decltype( value.runlevel);
                  case Enum::connecting: return "connecting";
                  case Enum::online: return "online";
                  case Enum::shutdown: return "shutdown";
                  case Enum::error: return "error";
                  case Enum::absent: return "absent";
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

            return terminal::format::formatter<  manager::admin::model::Connection>::construct( 
               terminal::format::column( "name", format_domain_name, terminal::color::yellow),
               terminal::format::column( "id", format_domain_id, terminal::color::blue),
               terminal::format::column( "bound", format_bound, terminal::color::magenta),
               terminal::format::column( "pid", format_pid, terminal::color::white, terminal::format::Align::right),
               terminal::format::column( "ipc", format_ipc, terminal::color::no_color, terminal::format::Align::right),
               terminal::format::column( "runlevel", format_runlevel, terminal::color::no_color),
               terminal::format::column( "local", format_local_address, terminal::color::blue),
               terminal::format::column( "peer", format_peer_address, terminal::color::blue)
            );
         }

         auto listeners() 
         {
            using vo = manager::admin::model::Listener;

            auto format_host = []( const vo& l){ return l.address.host;};
            auto format_port = []( const vo& l){ return l.address.port;};

            auto format_limit_size = []( const vo& l) -> std::string
            { 
               if( l.limit.size)
                  return std::to_string( l.limit.size);
               return "-";
            };

            auto format_limit_messages = []( const vo& l) -> std::string
            { 
               if( l.limit.messages)
                  return std::to_string( l.limit.messages);
               return "-";
            };

            return terminal::format::formatter< vo>::construct( 
               terminal::format::column( "host", format_host, terminal::color::yellow),
               terminal::format::column( "port", format_port, terminal::color::yellow),
               terminal::format::column( "limit size", format_limit_size, terminal::color::magenta, terminal::format::Align::right),
               terminal::format::column( "limit messages", format_limit_messages, terminal::color::magenta, terminal::format::Align::right)
            );
         }

      } // format

      namespace action
      {
         
         void list_connections()
         {
            format::connections().print( std::cout, call::state().connections);
         }

         void list_listeners()
         {
            format::listeners().print( std::cout, call::state().listeners);
         }

         void state( const std::optional< std::string>& format)
         {
            auto state = call::state();

            auto archive = common::serialize::create::writer::from( format.value_or( ""));
            archive << CASUAL_NAMED_VALUE( state);
            archive.consume( std::cout);
         }

         namespace rediscover
         {
            void invoke() 
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
            }

            constexpr auto description = R"(rediscover all outbound connections
            
)";
         } // rediscover

 

      } // action

      namespace manager
      {
         namespace admin 
         {
            struct cli::Implementation
            {
               common::argument::Group options()
               {
                  auto complete_state = []( auto values, bool){
                     return std::vector< std::string>{ "json", "yaml", "xml", "ini"};
                  };

                  return common::argument::Group{ [](){}, { "gateway"}, "gateway related administration",
                     common::argument::Option( &gateway::action::list_connections, { "-c", "--list-connections"}, "list all connections"),
                     common::argument::Option( &gateway::action::list_listeners, { "-l", "--list-listeners"}, "list all listeners"),
                     common::argument::Option( &gateway::action::rediscover::invoke, { "--rediscover"}, gateway::action::rediscover::description),
                     common::argument::Option( &gateway::action::state, complete_state, {"--state"}, "gateway state"),
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
   } // gateway
} // casual




