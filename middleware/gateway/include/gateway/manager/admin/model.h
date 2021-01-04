//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/macro.h"
#include "casual/platform.h"
#include "common/compare.h"

#include "common/domain.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace admin
         {
            namespace model
            {
               inline namespace v1
               {
                  namespace inbound
                  {
                     struct Limit : common::Compare< Limit>
                     {
                        platform::binary::size::type size = 0;
                        platform::binary::size::type messages = 0;
                        
                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( size);
                           CASUAL_SERIALIZE( messages);
                        })

                        inline auto tie() const noexcept { return std::tie( size, messages);}
                     };
                     
                  } // inbound
                  
                  struct Inbound
                  {
                     std::string alias;
                     common::process::Handle process;
                     inbound::Limit limit;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( alias);
                        CASUAL_SERIALIZE( process);
                        CASUAL_SERIALIZE( limit);
                     })
                  };

                  struct Outbound
                  {
                     std::string alias;
                     common::process::Handle process;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( alias);
                        CASUAL_SERIALIZE( process);
                     })
                  };


                  namespace connection
                  {
                     enum struct Bound : short
                     {
                        out,
                        in,
                        unknown,
                     };

                     inline std::ostream& operator << ( std::ostream& out, Bound value)
                     {
                        switch( value)
                        {
                           case Bound::out: return out << "out";
                           case Bound::in: return out << "in";
                           case Bound::unknown: return out << "unknown";
                        }
                        return out << "<unknown>";
                     }

                     enum struct Runlevel : short
                     {
                        absent,
                        connecting,
                        online,
                        shutdown,
                        error
                     };

                     inline std::ostream& operator << ( std::ostream& out, Runlevel value)
                     {
                        switch( value)
                        {
                           case Runlevel::absent: return out << "absent";
                           case Runlevel::connecting: return out << "connecting";
                           case Runlevel::online: return out << "online";
                           case Runlevel::shutdown: return out << "shutdown";
                           case Runlevel::error: return out << "error";
                        }
                        return out << "<unknown>";
                     }

                     struct Address
                     {
                        std::string local;
                        std::string peer;
                        
                        CASUAL_CONST_CORRECT_SERIALIZE(
                        {
                           CASUAL_SERIALIZE( local);
                           CASUAL_SERIALIZE( peer);
                        })
                     };
                     
                  } // connection

                  struct Connection : common::Compare< Connection>
                  {
                     connection::Bound bound = connection::Bound::unknown;
                     connection::Runlevel runlevel = connection::Runlevel::absent;
                     
                     std::string alias;
                     common::process::Handle process;
                     
                     common::domain::Identity remote;
                     connection::Address address;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( bound);
                        CASUAL_SERIALIZE( runlevel);
                        CASUAL_SERIALIZE( alias);
                        CASUAL_SERIALIZE( process);
                        CASUAL_SERIALIZE( remote);
                        CASUAL_SERIALIZE( address);
                     })

                     inline auto tie() const noexcept { return std::tie( remote, bound, process);}

                  };

                  namespace listener
                  {
                     struct Address : common::Compare< Address>
                     {
                        std::string host;
                        std::string port;

                        CASUAL_CONST_CORRECT_SERIALIZE(
                           CASUAL_SERIALIZE( host);
                           CASUAL_SERIALIZE( port);
                        )

                        inline auto tie() const noexcept { return std::tie( host, port);}
                     };

                  } // listener

                  struct Listener : common::Compare< Listener>
                  {
                     std::string alias;
                     inbound::Limit limit;
                     listener::Address address;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        CASUAL_SERIALIZE( alias);
                        CASUAL_SERIALIZE( limit);
                        CASUAL_SERIALIZE( address);
                     )

                     inline auto tie() const noexcept { return std::tie( alias, address, limit);}
                  };


                  struct State
                  {
                     std::vector< Inbound> inbounds;
                     std::vector< Outbound> outbounds;
                     std::vector< Connection> connections;
                     std::vector< Listener> listeners;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( inbounds);
                        CASUAL_SERIALIZE( outbounds);
                        CASUAL_SERIALIZE( connections);
                        CASUAL_SERIALIZE( listeners);
                     })
                  };

               } // v1
            } // model
         } // admin
      } // manager
   } // gateway
} // casual


