//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/manager/admin/cli.h"
#include "gateway/manager/admin/model.h"
#include "gateway/manager/admin/server.h"

#include "common/algorithm.h"
#include "casual/argument.h"
#include "common/terminal.h"
#include "common/chronology.h"
#include "common/transcode.h"
#include "common/exception/capture.h"
#include "common/event/listen.h"
#include "common/serialize/create.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/log.h"

#include "casual/cli/state.h"

#include "xatmi.h"

#include <iostream>

namespace casual
{
   using namespace common;

   namespace gateway::manager::admin::cli
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

            } // call


            namespace format
            {
               struct Resource : common::compare::Order< Resource>
               {
                  Resource( std::string resource, std::string name, common::strong::domain::id id, std::string peer)
                     : resource{ std::move( resource)}, name{ std::move( name)}, id{ id}, peer{ std::move( peer)} {}

                  std::string resource;
                  std::string name;
                  common::strong::domain::id id;
                  std::string peer;

                  inline auto tie() const noexcept { return std::tie( resource, name, id, peer);}

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( resource);
                     CASUAL_SERIALIZE( name);
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( peer);
                  )

               };

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

               auto group = []( auto& value)
               {
                  return value.group;
               };

               auto alias = []( auto& value)
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
                        return uuid::string( value.remote.id.value());
                     return "-";
                  };

                  auto format_bound = []( auto& value)
                  {
                     switch( value.bound)
                     {
                        using Enum = decltype( value.bound);
                        case Enum::in: return "in";
                        case Enum::in_forward: return "in*";
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

                  auto format_protocol = []( auto& value) -> std::string_view
                  {
                     if( value.protocol == decltype( value.protocol)::invalid)
                        return "-";
                     return description( value.protocol);
                  };

                  struct format_runlevel
                  {
                     std::size_t width( const model::Connection& value, const std::ostream&) const
                     {
                        using Enum = decltype( value.runlevel);
                        switch( value.runlevel)
                        {
                           case Enum::connecting: return 10;
                           case Enum::pending: return 7;
                           case Enum::connected: return 9;
                           case Enum::disconnecting: return 13;
                           case Enum::failed: return 6;
                           case Enum::disabled: return 8;
                        }
                        return 0;
                     }

                     void print( std::ostream& out, const model::Connection& value, std::size_t width) const
                     {
                        out << std::setfill( ' ') << std::left << std::setw( width);

                        using Enum = decltype( value.runlevel);
                        switch( value.runlevel)
                        {
                           case Enum::connecting: common::stream::write( out, terminal::color::white, value.runlevel); break;
                           case Enum::pending: common::stream::write( out, terminal::color::yellow, value.runlevel); break;
                           case Enum::connected: common::stream::write( out, terminal::color::green, value.runlevel); break;
                           case Enum::disconnecting: common::stream::write( out, terminal::color::magenta, value.runlevel); break;
                           case Enum::failed: common::stream::write( out, terminal::color::red, value.runlevel); break;
                           case Enum::disabled: common::stream::write( out, terminal::color::white, value.runlevel); break;
                        }
                     }
                  };

                  using Formatter = terminal::format::formatter<  manager::admin::model::Connection>;

                  if( ! terminal::output::directive().porcelain())
                  {
                     return Formatter::construct( 
                        terminal::format::column( "name", format_domain_name, terminal::color::yellow),
                        terminal::format::column( "id", format_domain_id, terminal::color::no_color),
                        terminal::format::column( "group", format::group, terminal::color::yellow),
                        terminal::format::column( "bound", format_bound, terminal::color::magenta),
                        terminal::format::custom::column( "runlevel", format_runlevel{}),
                        terminal::format::column( "P", format_protocol, terminal::color::no_color),
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
                        switch( value.runlevel)
                        {
                           using Enum = decltype( value.runlevel);
                           case Enum::connecting: return "connecting";
                           case Enum::pending: return "pending";
                           case Enum::connected: return "online";
                           case Enum::disconnecting: return "disconnecting";
                           case Enum::failed: return "failed";
                           case Enum::disabled: return "disabled";
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
                        terminal::format::column( "created", format::created),
                        terminal::format::column( "P", format_protocol)
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

                     struct format_runlevel
                     {
                        std::size_t width( const model::Listener& value, const std::ostream&) const
                        {
                           using Enum = decltype( value.runlevel);
                           switch( value.runlevel)
                           {
                              case Enum::listening: return 9;
                              case Enum::failed: return 6;
                              case Enum::disabled: return 8;
                           }
                           return 0;
                        }

                        void print( std::ostream& out, const model::Listener& value, std::size_t width) const
                        {
                           out << std::setfill( ' ') << std::left << std::setw( width);

                           using Enum = decltype( value.runlevel);
                           switch( value.runlevel)
                           {
                              case Enum::listening: common::stream::write( out, terminal::color::green, value.runlevel); break;
                              case Enum::failed: common::stream::write( out, terminal::color::red, value.runlevel); break;
                              case Enum::disabled: common::stream::write( out, terminal::color::white, value.runlevel); break;
                           }
                        }
                     };

                     return Formatter::construct( 
                        terminal::format::column( "group", format::group, terminal::color::yellow),
                        terminal::format::custom::column( "runlevel", format_runlevel{}),
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

                     auto format_runlevel = []( auto& value)
                     {
                        switch( value.runlevel)
                        {
                           using Enum = decltype( value.runlevel);
                           case Enum::listening: return "listening";
                           case Enum::failed: return "failed";
                           case Enum::disabled: return "disabled";
                        }
                        return "<unknown>";
                     };

                     return Formatter::construct( 
                        terminal::format::column( "host", format_host),
                        terminal::format::column( "port", format_port),
                        terminal::format::column( "limit size", format_limit_size),
                        terminal::format::column( "limit messages", format_limit_messages),
                        terminal::format::column( "group", format::group),
                        terminal::format::column( "created", format::created),
                        terminal::format::column( "runlevel", format_runlevel)
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
                  namespace connections
                  {
                     auto create()
                     {
                        auto invoke = []()
                        {
                           format::connections().print( std::cout, call::state().connections);
                        };

                        return argument::Option{ 
                           std::move( invoke),
                           argument::option::Names( { "-lc", "--list-connections"}, { "-c"}),
                           "list all connections"};
                     }

                     constexpr std::string_view legend = R"(
name:
   Name of the connected domain
id
   ID of the connected domain
group
   Alias of the group that hosts the connection
bound
   Direction of the connection.
     - out: from this domain to remote
     - in: from remote to this domain
     - in*: same as in, and discoveries are forwarded   
runlevel
   The runlevel the connection is in:
     - connecting
     - pending
     - connected
     - failed
P
   The protocol version that is used for the connection
local
   The local address for the connection
peer
   The peer address for the connection
created
   When the connection was established
)";

                  } // connections


                  auto listeners()
                  {
                     auto invoke = []()
                     {
                        format::listeners().print( std::cout, call::state().listeners);
                     };

                     return argument::Option{ 
                        std::move( invoke), 
                        argument::option::Names( { "-ll", "--list-listeners"}, { "-l"}), 
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

                  namespace resource
                  {
                     auto services()
                     {
                        auto invoke = []()
                        {
                           code::raise::error( code::casual::invalid_argument, "--list-services is removed - use casual service --list-instances");
                        };

                        return argument::Option{ 
                           std::move( invoke), 
                           argument::option::Names( {}, { "-ls", "--list-services"}), 
                           "removed - use casual service --list-instances"};
                     }

                     auto queues()
                     {
                        auto invoke = []()
                        {
                           code::raise::error( code::casual::invalid_argument, "--list-queues is removed - use casual queue --list-instances");
                        };

                        return argument::Option{ 
                           std::move( invoke), 
                           argument::option::Names( {}, { "-ls", "--list-services"}), 
                           "removed - use casual queue --list-instance"};
                     }
                  }
               } // list

               auto rediscover()
               {
                  auto invoke = []()
                  {
                     std::cerr << "use casual discover --rediscover instead\n";
                  };

                  return argument::Option{ 
                     std::move( invoke),
                     argument::option::Names( {}, { "--rediscover"}), 
                     "moved to casual discover --rediscover"};
               }

               namespace legend
               {
                  const std::map< std::string_view, std::string_view> legends{
                     { "list-connections", option::list::connections::legend}
                  };

                  auto create()
                  {
                     auto invoke = []( const std::string& option)
                     {
                        if( auto found = algorithm::find( legend::legends, option))
                           std::cout << found->second;
                        else
                           code::raise::error( code::casual::invalid_argument, "not a valid argument to --legend: ", option);
                     };

                     auto complete = []( auto values, auto help)
                     {
                        return algorithm::transform( legends, []( auto& pair){ return std::string{ pair.first};});
                     };

                     return argument::Option{ 
                        std::move( invoke),
                        complete,
                        { "--legend"}, 
                        R"(show legend for the output of the supplied option

Documentation and description for abbreviations and acronyms used as columns in output

note: not all options has legend, use 'auto complete' to find out which legends are supported.                        
)"};
                  }

               } // legend

            } // option
         } // <unnamed>
      } // local

      argument::Option options()
      {
         return argument::Option{ [](){}, { "gateway"}, "gateway related administration"}( {
            local::option::list::connections::create(),
            local::option::list::listeners(),
            local::option::list::resource::services(),
            local::option::list::resource::queues(),
            local::option::list::groups::inbound(),
            local::option::list::groups::outbound(),
            local::option::legend::create(),
            casual::cli::state::option( &local::call::state),

            local::option::rediscover() // removed... TODO: remove in 2.0
         });

      }
            
 
   } // gateway::manager::admin::cli
} // casual




