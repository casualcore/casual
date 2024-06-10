//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "gateway/message/protocol.h"

#include "common/serialize/macro.h"
#include "casual/platform.h"
#include "common/message/type.h"
#include "common/compare.h"
#include "common/transaction/id.h"

#include "common/domain.h"

namespace casual
{
   namespace gateway::manager::admin::model
   {
      inline namespace v3
      {
         namespace connection
         {
            enum struct Phase : short
            {
               regular,
               reversed,
            };

            constexpr std::string_view description( Phase value) noexcept
            {
               switch( value)
               {
                  case Phase::regular: return "regular";
                  case Phase::reversed: return "reversed";
               }
               return "<unknown>";
            }


            enum struct Bound : short
            {
               unknown,
               out,
               in,
               in_forward,
            };

            constexpr std::string_view description( Bound value) noexcept
            {
               switch( value)
               {
                  case Bound::out: return "out";
                  case Bound::in: return "in";
                  case Bound::in_forward: return "in*";
                  case Bound::unknown: return "unknown";
               }
               return "<unknown>";
            }


            //! TODO maintenance use default values...
            enum struct Runlevel : short
            {
               connecting = 1,
               pending = 4,
               connected = 2,
               failed = 3,

               //! @deprecated remove in 2.0
               online = connected,
            };
            constexpr std::string_view description( Runlevel value) noexcept
            {
               switch( value)
               {
                  case Runlevel::connecting: return "connecting";
                  case Runlevel::pending: return "pending";
                  case Runlevel::connected: return "connected";
                  case Runlevel::failed: return "failed";
               }
               return "<unknown>";
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
            connection::Runlevel runlevel = connection::Runlevel::connecting;
            connection::Phase connect{};
            connection::Bound bound{};
            gateway::message::protocol::Version protocol{};
            common::strong::file::descriptor::id descriptor{};
            common::domain::Identity remote;
            connection::Address address;
            common::strong::ipc::id ipc;
            platform::time::point::type created{};
            
            
            inline friend bool operator == ( const Connection& lhs, std::string_view rhs) { return lhs.remote == rhs;}

            //! @deprecated remove in 2.0 - group knows the process, not the connection
            common::process::Handle process;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( group);
               CASUAL_SERIALIZE( runlevel);
               CASUAL_SERIALIZE( connect);
               CASUAL_SERIALIZE( bound);
               CASUAL_SERIALIZE( protocol);
               CASUAL_SERIALIZE( descriptor);
               CASUAL_SERIALIZE( remote);
               CASUAL_SERIALIZE( address);
               CASUAL_SERIALIZE( ipc);
               CASUAL_SERIALIZE( created);
            
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
            constexpr std::string_view description( Runlevel value) noexcept
            {
               switch( value)
               {
                  case Runlevel::running: return "running";
                  case Runlevel::shutdown: return "shutdown";
                  case Runlevel::error: return "error";
               }
               return "<unknown>";
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

               inline friend bool operator == ( const Group& lhs, std::string_view rhs) { return lhs.alias == rhs;}

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
               struct Task
               {
                  common::strong::correlation::id correlation;
                  common::strong::file::descriptor::id connection;
                  std::vector< common::message::Type> message_types;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( correlation);
                     CASUAL_SERIALIZE( connection);
                     CASUAL_SERIALIZE( message_types);
                  )
               };

               struct Transaction
               {
                  common::transaction::global::ID gtrid;
                  std::vector< common::strong::socket::id> connections;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     CASUAL_SERIALIZE( gtrid);
                     CASUAL_SERIALIZE( connections);
                  )
               };
            } // pending

            struct Pending
            {
               std::vector< pending::Task> tasks;
               std::vector< pending::Transaction> transactions;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( tasks);
                  CASUAL_SERIALIZE( transactions);
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

               inline friend bool operator == ( const Group& lhs, std::string_view rhs) { return lhs.alias == rhs;}

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

            constexpr std::string_view description( Runlevel value) noexcept
            {
               switch ( value)
               {
                  case Runlevel::listening: return "listening";
                  case Runlevel::failed: return "failed";
               }
               return "<not used>";
            }

         } // listener

         struct Listener : common::Compare< Listener>
         {
            std::string group;
            listener::Runlevel runlevel{};
            listener::Address address;
            connection::Bound bound{};
            platform::time::point::type created{};
         
            //@ deprecated
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
               CASUAL_SERIALIZE( runlevel);
               CASUAL_SERIALIZE( address);
               CASUAL_SERIALIZE( bound);
               CASUAL_SERIALIZE( created);
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


