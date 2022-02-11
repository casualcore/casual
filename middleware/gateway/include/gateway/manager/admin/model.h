//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/macro.h"
#include "casual/platform.h"
#include "common/message/type.h"
#include "common/compare.h"

#include "common/domain.h"

namespace casual
{
   namespace gateway::manager::admin::model
   {
      inline namespace v2
      {
         namespace connection
         {
            enum struct Phase : short
            {
               regular,
               reversed,
            };

            inline std::ostream& operator << ( std::ostream& out, Phase value)
            {
               switch( value)
               {
                  case Phase::regular: return out << "regular";
                  case Phase::reversed: return out << "reversed";
               }
               return out << "<unknown>";
            }


            enum struct Bound : short
            {
               unknown,
               out,
               in,
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
               connecting = 1,
               connected = 2,
               failed = 3,
               //! @deprecated remove in 2.0
               online = connected,
            };

            inline std::ostream& operator << ( std::ostream& out, Runlevel value)
            {
               switch ( value)
               {
                  case Runlevel::connecting: return out << "connecting";
                  case Runlevel::connected: return out << "connected";
                  case Runlevel::failed: return out << "failed";
               }
               return out << "<not used>";
            }

            struct Address
            {
               std::string local;
               std::string peer;
               
               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( local);
                  CASUAL_SERIALIZE( peer);
               )
            };
            
         } // connection

         struct Connection : common::Compare< Connection>
         {
            std::string group;
            connection::Phase connect{};
            connection::Bound bound{};
            common::strong::file::descriptor::id descriptor{};
            common::domain::Identity remote;
            connection::Address address;
            platform::time::point::type created{};
            connection::Runlevel runlevel{};

            //! @deprecated remove in 2.0 - group knows the process, not the connection
            common::process::Handle process;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( group);
               CASUAL_SERIALIZE( connect);
               CASUAL_SERIALIZE( bound);
               CASUAL_SERIALIZE( descriptor);
               CASUAL_SERIALIZE( remote);
               CASUAL_SERIALIZE( address);
               CASUAL_SERIALIZE( created);
               CASUAL_SERIALIZE( runlevel);

               //! @deprecated remove in 2.0
               CASUAL_SERIALIZE( process);
            )

            inline auto tie() const noexcept { return std::tie( remote, bound, group);}
         };

         namespace group
         {
            enum struct Runlevel : short
            {
               running,
               shutdown,
               error,
            };

            inline std::ostream& operator << ( std::ostream& out, Runlevel value)
            {
               switch( value)
               {
                  case Runlevel::running: return out << "running";
                  case Runlevel::shutdown: return out << "shutdown";
                  case Runlevel::error: return out << "error";
               }
               return out << "<unknown>";
            }

         } // group

         namespace inbound
         {
            struct Limit
            {
               platform::binary::size::type size = 0;
               platform::binary::size::type messages = 0;
               
               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( size);
                  CASUAL_SERIALIZE( messages);
               )
            };

            struct Group
            {
               std::string alias;
               group::Runlevel runlevel{};
               connection::Phase connect{};
               common::process::Handle process;
               inbound::Limit limit{};
               std::string note;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( runlevel);
                  CASUAL_SERIALIZE( connect);
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( limit);
                  CASUAL_SERIALIZE( note);
               )
            };
         } // inbound

         namespace outbound
         {
            namespace pending
            {
               struct Message
               {
                  common::message::Type type{};
                  platform::size::type count{};

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( type);
                     CASUAL_SERIALIZE( count);
                  )
               };
            } // pending

            struct Pending
            {
               std::vector< pending::Message> messages;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( messages);
               )
            };

            struct Group
            {
               std::string alias;
               group::Runlevel runlevel{};
               connection::Phase connect{};
               common::process::Handle process;
               platform::size::type order{};
               Pending pending;
               std::string note;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( runlevel);
                  CASUAL_SERIALIZE( connect);
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( order);
                  CASUAL_SERIALIZE( pending);
                  CASUAL_SERIALIZE( note);
               )
            };
         } // outbound

 

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

            enum struct Runlevel : short
            {
               listening = 1,
               failed = 2,
            };

            inline std::ostream& operator << ( std::ostream& out, Runlevel value)
            {
               switch ( value)
               {
                  case Runlevel::listening: return out << "listening";
                  case Runlevel::failed: return out << "failed";
               }
               return out << "<not used>";
            }

         } // listener

         struct Listener : common::Compare< Listener>
         {
            std::string group;
            listener::Address address;
            connection::Bound bound{};
            platform::time::point::type created{};
            listener::Runlevel runlevel{};

            //@ deprecared
            struct
            {
               std::string note = "NOT USED";
               platform::binary::size::type size = 0;
               platform::binary::size::type messages = 0;
               
               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( note);
                  CASUAL_SERIALIZE( size);
                  CASUAL_SERIALIZE( messages);
               )
             } limit;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( group);
               CASUAL_SERIALIZE( address);
               CASUAL_SERIALIZE( bound);
               CASUAL_SERIALIZE( created);
               CASUAL_SERIALIZE( runlevel);
               CASUAL_SERIALIZE( limit);
            )

            inline auto tie() const noexcept { return std::tie( group, address);}
         };

         namespace connection
         {
            struct Identifier
            {
               common::strong::process::id pid{};
               common::strong::file::descriptor::id descriptor{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( pid);
                  CASUAL_SERIALIZE( descriptor);
               )
            };
         }

         struct Routing : common::Compare< Routing>
         {
            std::string name;
            std::vector< connection::Identifier> connections;

            inline friend bool operator == ( const Routing& lhs, std::string_view name) { return lhs.name == name;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( connections);
            )
            inline auto tie() const noexcept { return std::tie( name);}

         };


         struct State
         {
            struct
            {
               std::vector< inbound::Group> groups;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( groups);
               )

            } inbound;

            struct
            {
               std::vector< outbound::Group> groups;
               
               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( groups);
               )

            } outbound;

            std::vector< Connection> connections;
            std::vector< Listener> listeners;
            std::vector< Connection> failed_connections;
            std::vector< Listener> failed_listeners;

            std::vector< Routing> services;
            std::vector< Routing> queues;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( inbound);
               CASUAL_SERIALIZE( outbound);
               CASUAL_SERIALIZE( connections);
               CASUAL_SERIALIZE( listeners);
               CASUAL_SERIALIZE( failed_connections);
               CASUAL_SERIALIZE( failed_listeners);
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( queues);
            )
         };

      } // v2
      
   } // gateway::manager::admin::model
} // casual


