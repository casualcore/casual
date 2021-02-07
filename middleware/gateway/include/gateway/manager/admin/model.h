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

            std::ostream& operator << ( std::ostream& out, Phase value);

            enum struct Bound : short
            {
               unknown,
               out,
               in,
            };

            std::ostream& operator << ( std::ostream& out, Bound value);

            enum struct Runlevel : short
            {
               connecting = 1,
               connected = 2,
               //! @deprecated remove in 2.0
               online = connected,
            };

            std::ostream& operator << ( std::ostream& out, Runlevel value);

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
            common::domain::Identity remote;
            connection::Address address;
            platform::time::point::type created{};

            inline connection::Runlevel runlevel() const noexcept
            {
               if( ! address.peer.empty())
                  return connection::Runlevel::connected;
               return connection::Runlevel::connecting;
            }

            //! @deprecated remove in 2.0 - group knows the process, not the connection
            common::process::Handle process;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( group);
               CASUAL_SERIALIZE( connect);
               CASUAL_SERIALIZE( bound);
               CASUAL_SERIALIZE( remote);
               CASUAL_SERIALIZE( address);
               CASUAL_SERIALIZE( created);

               //! @deprecated remove in 2.0
               auto runlevel = Connection::runlevel();
               CASUAL_SERIALIZE( runlevel);
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

            std::ostream& operator << ( std::ostream& out, Runlevel value);

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
            struct Group
            {
               std::string alias;
               group::Runlevel runlevel{};
               connection::Phase connect{};
               common::process::Handle process;
               platform::size::type order{};
               std::string note;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( alias);
                  CASUAL_SERIALIZE( runlevel);
                  CASUAL_SERIALIZE( connect);
                  CASUAL_SERIALIZE( process);
                  CASUAL_SERIALIZE( order);
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

         } // listener

         struct Listener : common::Compare< Listener>
         {
            std::string group;
            listener::Address address;
            connection::Bound bound{};
            platform::time::point::type created{};

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
               CASUAL_SERIALIZE( limit);
            )

            inline auto tie() const noexcept { return std::tie( group, address);}
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

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( inbound);
               CASUAL_SERIALIZE( outbound);
               CASUAL_SERIALIZE( connections);
               CASUAL_SERIALIZE( listeners);
            )
         };

      } // v2
      
   } // gateway::manager::admin::model
} // casual


