//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/outbound/state/route.h"
#include "gateway/message.h"

#include "common/serialize/macro.h"
#include "common/communication/select.h"
#include "common/communication/tcp.h"
#include "common/domain.h"
#include "common/message/coordinate.h"
#include "common/message/gateway.h"
#include "common/state/machine.h"

#include "configuration/model.h"

#include <string>
#include <vector>

namespace casual
{
   namespace gateway::outbound
   {
      namespace state
      {

         enum struct Runlevel : short
         {
            running,
            shutdown,
            error,
         };

         std::ostream& operator << ( std::ostream& out, Runlevel value);

         namespace external
         {
            struct Connection
            {
               inline explicit Connection( common::communication::tcp::Duplex&& device)
                  : device{ std::move( device)} {}

               common::communication::tcp::Duplex device;

               inline friend bool operator == ( const Connection& lhs, common::strong::file::descriptor::id rhs) 
               { 
                  return lhs.device.connector().descriptor() == rhs;
               }
               
               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( device);
               )
            };

            namespace connection
            {
               struct Information
               {
                  common::strong::file::descriptor::id connection;
                  common::domain::Identity domain;
                  configuration::model::gateway::outbound::Connection configuration;

                  inline friend bool operator == ( const Information& lhs, common::strong::file::descriptor::id rhs) { return lhs.connection == rhs;} 

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( connection);
                     CASUAL_SERIALIZE( domain);
                     CASUAL_SERIALIZE( configuration);
                  )
               };
            } // connection

         } // external

         struct External
         {
            inline void add( 
               common::communication::select::Directive& directive, 
               common::communication::tcp::Duplex&& device, 
               configuration::model::gateway::outbound::Connection configuration)
            {
               auto descriptor = device.connector().descriptor();

               connections.emplace_back( std::move( device));
               descriptors.push_back( descriptor);
               information.push_back( [&](){
                  state::external::connection::Information result;
                  result.connection = descriptor;
                  result.configuration = std::move( configuration);
                  return result;
               }());
               directive.read.add( descriptor);
               
            }

            inline state::external::Connection* connection( common::strong::file::descriptor::id descriptor)
            {
               if( auto found = common::algorithm::find( connections, descriptor))
                  return found.data();
               return nullptr;
            }

            inline std::optional< configuration::model::gateway::outbound::Connection> remove( 
               common::communication::select::Directive& directive, 
               common::strong::file::descriptor::id descriptor)
            {
               directive.read.remove( descriptor);
               common::algorithm::trim( connections, common::algorithm::remove( connections, descriptor));
               common::algorithm::trim( descriptors, common::algorithm::remove( descriptors, descriptor));
               if( auto found = algorithm::find( information, descriptor))
                  return common::algorithm::extract( information, std::begin( found)).configuration;
               
               return {};
            }

            inline void clear( common::communication::select::Directive& directive)
            {
               directive.read.remove( descriptors);
               connections.clear();
               descriptors.clear();
               information.clear();
            }

            std::vector< state::external::Connection> connections;
            std::vector< common::strong::file::descriptor::id> descriptors;
            std::vector< state::external::connection::Information> information;

            //! holds the last external connection that was used
            common::strong::file::descriptor::id last;

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( connections);
               CASUAL_SERIALIZE( descriptors);
               CASUAL_SERIALIZE( information);
               CASUAL_SERIALIZE( last);
            )

         private:
            
         };

         struct Lookup
         {
            struct Mapping
            {
               inline Mapping( const common::transaction::ID& internal)
                  : internal{ internal} {}

               
               struct External
               {
                  External() = default;
                  inline External( common::strong::file::descriptor::id connection, const common::transaction::ID& trid)
                     : connection{ connection}, trid{ trid} {}

                  common::strong::file::descriptor::id connection;
                  common::transaction::ID trid;

                  inline friend bool operator == ( const External& lhs, common::strong::file::descriptor::id rhs) { return lhs.connection == rhs;}
                  inline friend bool operator == ( const External& lhs, const common::transaction::ID& rhs) { return lhs.trid == rhs;}

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( connection);
                     CASUAL_SERIALIZE( trid);
                  )
               };

               common::transaction::ID internal;
               std::vector< External> externals;

               const External& branch( common::strong::file::descriptor::id connection);

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( internal);
                  CASUAL_SERIALIZE( externals);
               )
            };

            std::unordered_map< std::string, std::vector< common::strong::file::descriptor::id>> services;
            std::vector< Mapping> transactions;
            std::unordered_map< std::string, std::vector< common::strong::file::descriptor::id>> queues;

            using Result = std::tuple< Mapping::External, bool>;

            Result service( const std::string& service, const common::transaction::ID& trid);
            Result queue( const std::string& queue, const common::transaction::ID& trid);

            
            const common::transaction::ID& external( const common::transaction::ID& internal, common::strong::file::descriptor::id connection) const;
            const common::transaction::ID& internal( const common::transaction::ID& external) const;

            //! @returns the associated connection from the external branch
            common::strong::file::descriptor::id connection( const common::transaction::ID& external) const;


            struct Resources
            {
               std::vector< std::string> services;
               std::vector< std::string> queues;

               inline explicit operator bool() const noexcept { return ! services.empty() || ! queues.empty();}

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( queues);
               )
            };

            //! @returns all lookup resources
            Resources resources() const;


            Resources add( common::strong::file::descriptor::id descriptor, std::vector< std::string> services, std::vector< std::string> queues);

            //! removes the connection and @return the resources that should be un-advertised 
            Resources remove( common::strong::file::descriptor::id descriptor);

            //! remove the connection for the provided services and queues, @returns all that needs to be un-advertised.
            Resources remove( common::strong::file::descriptor::id descriptor, std::vector< std::string> services, std::vector< std::string> queues);

            //! clears all 'resources' but keep the transaction mapping (if there are messages in flight).
            //! @returns all resources that should be unadvertised
            Resources clear();
            
            void remove( const common::transaction::ID& internal);

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( transactions);
               CASUAL_SERIALIZE( queues);
            )
         };

      } // state

      struct State
      {
         common::state::Machine< state::Runlevel, state::Runlevel::running> runlevel;
         common::communication::select::Directive directive;
         state::External external;

         struct 
         {
            struct
            {
               state::route::service::Message message;
               common::message::event::service::Calls metric{ common::process::handle()};

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( message);
                  CASUAL_SERIALIZE( metric);
               )
            } service;

            state::route::Message message;

            inline bool empty() const { return message.empty() && service.message.empty();}

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( service);
               CASUAL_SERIALIZE( message);
            )
         } route;
         
         state::Lookup lookup;

         //! holds all connections that has been requested to disconnect.
         std::vector< common::strong::file::descriptor::id> disconnecting;
         
         struct
         {
            common::message::coordinate::fan::Out< common::message::gateway::domain::discover::Reply, common::strong::file::descriptor::id> discovery;

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( discovery);
            )
         } coordinate;

         std::string alias;
         platform::size::type order{};

         bool done() const;

         //! @returns a reply message to state `request` that is filled with what's possible
         template< typename M>
         auto reply( M&& request)
         {
            auto reply = common::message::reverse::type( request, common::process::handle());
            reply.state.alias = alias;
            reply.state.order = order;

            reply.state.connections = common::algorithm::transform( external.connections, [&]( auto& connection)
            {
               auto descriptor = connection.device.connector().descriptor();
               message::outbound::state::Connection result;
               result.descriptor = descriptor;
               result.address.local = common::communication::tcp::socket::address::host( descriptor);
               result.address.peer = common::communication::tcp::socket::address::peer( descriptor);

               if( auto found = common::algorithm::find( external.information, descriptor))
               {
                  result.domain = found->domain;
               }

               return result;
            });
            return reply;
         }
         

         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( external);
            CASUAL_SERIALIZE( route);
            CASUAL_SERIALIZE( lookup);
            CASUAL_SERIALIZE( disconnecting);
            CASUAL_SERIALIZE( coordinate);
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( order);
         )
      };

      } // gateway::outbound
} // casual
