//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/reverse/outbound/state/route.h"
#include "gateway/reverse/outbound/state/coordinate.h"

#include "common/serialize/macro.h"
#include "common/communication/select.h"
#include "common/communication/tcp.h"
#include "common/domain.h"
#include "common/message/service.h"

#include "configuration/model.h"

#include <string>
#include <vector>

namespace casual
{
   namespace gateway::reverse::outbound
   {
      namespace state
      {
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
                  configuration::model::gateway::reverse::outbound::Connection configuration;

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
               configuration::model::gateway::reverse::outbound::Connection configuration)
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

            std::vector< state::external::Connection> connections;
            std::vector< common::strong::file::descriptor::id> descriptors;
            std::vector< state::external::connection::Information> information;

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( connections);
               CASUAL_SERIALIZE( descriptors);
               CASUAL_SERIALIZE( information);
            )
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

            struct Advertise
            {
               std::vector< std::string> services;
               std::vector< std::string> queues;
            };


            Advertise add( common::strong::file::descriptor::id descriptor, std::vector< std::string> services, std::vector< std::string> queues);

            //! removes the connection and @return the resources that should be un-advertised 
            Advertise remove( common::strong::file::descriptor::id descriptor);

            //! remove the connection for the provided services and queues, @returns all that needs to be un-advertised.
            Advertise remove( common::strong::file::descriptor::id descriptor, std::vector< std::string> services, std::vector< std::string> queues);
            
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
         common::communication::select::Directive directive;
         state::External external;

         struct 
         {
            struct
            {
               state::route::service::Message message;
               common::message::event::service::Calls metric;

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( message);
                  CASUAL_SERIALIZE( metric);
               )
            } service;

            state::route::Message message;

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( service);
               CASUAL_SERIALIZE( message);
            )
         } route;
         
         state::Lookup lookup;
         
         struct
         {
            state::coordinate::Discovery discovery;

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( discovery);
            )
         } coordinate;

         std::string alias;
         platform::size::type order{};
         

         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( external);
            CASUAL_SERIALIZE( route);
            CASUAL_SERIALIZE( lookup);
            CASUAL_SERIALIZE( coordinate);
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( order);
         )
      };

      } // gateway::reverse::outbound
} // casual
