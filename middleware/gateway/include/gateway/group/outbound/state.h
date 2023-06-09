//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/outbound/state/route.h"
#include "gateway/group/tcp.h"
#include "gateway/message.h"

#include "common/serialize/macro.h"
#include "common/communication/select.h"
#include "common/communication/tcp.h"
#include "common/communication/ipc/send.h"
#include "common/domain.h"
#include "common/message/coordinate.h"
#include "common/state/machine.h"
#include "common/process.h"
#include "common/algorithm.h"

#include "configuration/model.h"

#include <string>
#include <vector>

namespace casual
{
   namespace gateway::group::outbound
   {
      struct Policy
      {
         struct next
         {
            //! max count of consumed messages 
            static constexpr platform::size::type ipc = 1;
            //! max count of consumed messages
            static constexpr platform::size::type tcp = 1;
         };
      };

      namespace state
      {

         enum struct Runlevel : short
         {
            running,
            shutdown,
            error,
         };
         std::string_view description( Runlevel value);

         namespace lookup
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

            namespace resource
            {
               struct Connection
               {
                  inline Connection( common::strong::file::descriptor::id id, platform::size::type hops) : id{ id}, hops{ hops} {}

                  common::strong::file::descriptor::id id;
                  platform::size::type hops{};

                  // conversion operator to pretend to be a pure "id"
                  inline operator common::strong::file::descriptor::id() const noexcept { return id;}

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( id);
                     CASUAL_SERIALIZE( hops);
                  )
               };
            } // resource


            struct Resource
            {
               std::string name;
               platform::size::type hops;
            };

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
            
         } // lookup

         struct Lookup
         {

            using Result = std::tuple< lookup::Mapping::External, bool>;

            Result service( const std::string& service, const common::transaction::ID& trid);
            Result queue( const std::string& queue, const common::transaction::ID& trid);

            
            const common::transaction::ID& external( const common::transaction::ID& internal, common::strong::file::descriptor::id connection) const;
            const common::transaction::ID& internal( const common::transaction::ID& external) const;

            //! @returns the associated connection from the external branch
            common::strong::file::descriptor::id connection( const common::transaction::ID& external) const;

            //! @returns all lookup resources
            lookup::Resources resources() const;

            lookup::Resources add( common::strong::file::descriptor::id descriptor, std::vector< lookup::Resource> services, std::vector< lookup::Resource> queues);

            //! removes the connection and @return the resources that should be un-advertised 
            lookup::Resources remove( common::strong::file::descriptor::id descriptor);

            //! remove the connection for the provided services and queues, @returns all that needs to be un-advertised.
            lookup::Resources remove( common::strong::file::descriptor::id descriptor, std::vector< std::string> services, std::vector< std::string> queues);

            //! clears all 'resources' but keep the transaction mapping (if there are messages in flight).
            //! @returns all resources that should be unadvertised
            lookup::Resources clear();
            
            //! removes the "mapping", based on the external (branched) trid.
            void remove( const common::transaction::ID& external);

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE_NAME( m_services, "services");
               CASUAL_SERIALIZE_NAME( m_transactions, "transactions");
               CASUAL_SERIALIZE_NAME( m_queues, "queues");
            )

            inline auto& services() const noexcept { return m_services;}
            inline auto& transactions() const noexcept { return m_transactions;}
            inline auto& queues() const noexcept { return m_queues;}

         private:

            std::unordered_map< std::string, std::vector< lookup::resource::Connection>> m_services;
            std::vector< lookup::Mapping> m_transactions;
            std::unordered_map< std::string, std::vector< lookup::resource::Connection>> m_queues;
         };

         namespace service
         {
            struct Pending
            {
               struct Call
               {
                  Call( const common::message::service::call::callee::Request& message)
                     : correlation{ message.correlation}, service{ message.service.requested.value_or( message.service.name)}, parent{ message.parent}
                  {}

                  common::strong::correlation::id correlation;
                  std::string service;
                  std::string parent;
                  platform::time::point::type start = platform::time::clock::type::now();

                  inline friend bool operator == ( const Call& lhs, const common::strong::correlation::id& rhs) { return lhs.correlation == rhs;}
               };

               inline void add( Call call) { m_calls.push_back( std::move( call));}

               inline void add( const common::message::service::call::callee::Request& message)
               {
                  assert( ! common::algorithm::find( m_calls, message.correlation));
                  m_calls.emplace_back( message);
               }


               inline auto consume( const common::strong::correlation::id& correlation)
               {
                  auto found = common::algorithm::find( m_calls, correlation);
                  assert( found);
                  return common::algorithm::container::extract( m_calls, std::begin( found));
               }

               inline void add( common::message::event::service::Metric metric)
               {
                  m_metric.metrics.push_back( std::move( metric));
               }
               
               //! "force" metric callback if there are metric to send
               template< typename State, typename CB>
               void force_metric( State& state, CB&& callback)
               {
                  if( m_metric.metrics.empty())
                     return;
                  
                  callback( state, m_metric);
                  m_metric.metrics.clear();
               }

               //! send service metrics if we don't have any more in-flight call request (this one
               //! was the last, or only) OR we've accumulated enough metrics for a batch update               
               template< typename State, typename CB>
               void maybe_metric( State& state, CB&& callback)
               {
                  if( m_calls.empty() || m_metric.metrics.size() >= platform::batch::gateway::metrics)
                     force_metric( state, std::forward< CB>( callback));
               }

               inline explicit operator bool() const noexcept { return ! m_calls.empty();}

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE_NAME( m_calls, "calls");
                  CASUAL_SERIALIZE_NAME( m_metric, "metric");
               )

            private:
               std::vector< Call> m_calls;
               common::message::event::service::Calls m_metric{ common::process::handle()};
            };

            
            
         } // service

      } // state

      struct State
      {
         common::state::Machine< state::Runlevel, state::Runlevel::running> runlevel;
         common::communication::select::Directive directive;
         group::tcp::External< configuration::model::gateway::outbound::Connection> external;

         state::Route route;

         state::service::Pending service;
         state::Lookup lookup;

         common::communication::ipc::send::Coordinator multiplex{ directive};
         
         struct
         {
            common::message::coordinate::fan::Out< casual::domain::message::discovery::Reply, common::strong::file::descriptor::id> discovery;

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( discovery);
            )
         } coordinate;

         //! holds all connections that has been requested to disconnect.
         std::vector< common::strong::file::descriptor::id> disconnecting;

         std::string alias;
         platform::size::type order{};

         bool done() const;

         //! @returns a reply message to state `request` that is filled with what's possible
         template< typename M>
         auto reply( M&& request) const noexcept
         {
            auto reply = external.reply( request);

            reply.state.alias = alias;
            reply.state.order = order;

            auto transform = []( auto resource)
            {
               using Identifier = message::outbound::state::connection::Identifier;
               auto pid = common::process::id();
               auto& name = resource.first;
               auto& descriptors = resource.second;
               std::vector< Identifier> identifiers;

               common::algorithm::transform( descriptors, identifiers, [pid]( auto descriptor){
                  return Identifier{ pid, descriptor};
               });

               return typename decltype(reply.state.correlation.services)::value_type{ name, identifiers};
            };

            common::algorithm::transform( lookup.services(), reply.state.correlation.services, transform);
            common::algorithm::transform( lookup.queues(), reply.state.correlation.queues, transform);

            // pending
            reply.state.pending.messages = common::algorithm::transform( route.points(), []( auto& point)
            {
               message::outbound::state::pending::Message result;
               result.connection = point.connection;
               result.correlation = point.destination.correlation;
               result.target = point.destination.ipc;
               return result;
            });

            reply.state.pending.transactions = common::algorithm::transform( lookup.transactions(), []( auto& transaction)
            {
               message::outbound::state::pending::Transaction result;
               result.internal = transaction.internal;
               result.externals = common::algorithm::transform( transaction.externals, []( auto& external)
               {
                  message::outbound::state::pending::Transaction::External result;
                  result.trid = external.trid;
                  result.connection = external.connection;
                  return result;
               });
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

   } // gateway::group::outbound
} // casual
