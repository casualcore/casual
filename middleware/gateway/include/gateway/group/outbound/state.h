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
            static constexpr platform::size::type ipc() { return 1;}
            //! max count of consumed messages
            static constexpr platform::size::type tcp() { return 1;}
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
            namespace resource
            {
               struct Connection
               {
                  inline Connection( common::strong::socket::id id, platform::size::type hops) : id{ id}, hops{ hops} {}

                  common::strong::socket::id id;
                  platform::size::type hops{};

                  // conversion operator to pretend to be a pure "id"
                  inline operator common::strong::socket::id() const noexcept { return id;}

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

            struct Result
            {
               common::strong::socket::id connection;
               bool new_transaction = true;

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( connection);
                  CASUAL_SERIALIZE( new_transaction);
               )
            };
            
         } // lookup

         struct Lookup
         {

            using descriptor_range = common::range::const_type_t< std::vector< common::strong::socket::id>>;

            lookup::Result service( const std::string& service, const common::transaction::ID& trid);
            lookup::Result queue( const std::string& queue, const common::transaction::ID& trid);


            //! @returns the associated connections from the gtrid, if any.
            descriptor_range connections( common::transaction::global::id::range gtrid) const noexcept;

            //! @returns all lookup resources
            lookup::Resources resources() const;

            lookup::Resources add( common::strong::socket::id descriptor, std::vector< lookup::Resource> services, std::vector< lookup::Resource> queues);

            //! removes the connection and @return the resources that should be un-advertised 
            lookup::Resources remove( common::strong::socket::id descriptor);

            //! remove the connection for the provided services and queues, @returns all that needs to be un-advertised.
            lookup::Resources remove( common::strong::socket::id descriptor, std::vector< std::string> services, std::vector< std::string> queues);

            //! removes the `gtrid` association with the `descriptor`
            void remove( common::transaction::global::id::range gtrid, common::strong::socket::id descriptor);

            //! indic
            std::vector< common::transaction::global::ID> failed( common::strong::socket::id descriptor);
            
            //! removes the mapping associated with the `gtrid`
            void remove( common::transaction::global::id::range gtrid);

            inline auto& services() const noexcept { return m_services;}
            inline auto& transactions() const noexcept { return m_transactions;}
            inline auto& queues() const noexcept { return m_queues;}

            inline bool empty() const noexcept { return m_services.empty() && m_transactions.empty() && m_queues.empty();}

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE_NAME( m_services, "services");
               CASUAL_SERIALIZE_NAME( m_transactions, "transactions");
               CASUAL_SERIALIZE_NAME( m_queues, "queues");
            )

         private:
            std::unordered_map< std::string, std::vector< lookup::resource::Connection>> m_services;
            std::unordered_map< common::transaction::global::ID, std::vector< common::strong::socket::id>, common::transaction::global::hash, std::equal_to<>> m_transactions;
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

         namespace extract
         {
            struct Result
            {
               group::tcp::External< configuration::model::gateway::outbound::Connection>::Information information;
               std::vector< route::Point> routes;
               std::vector< common::transaction::global::ID> gtrids;

               inline bool empty() const noexcept { return routes.empty() && gtrids.empty();}

               CASUAL_LOG_SERIALIZE( 
                 CASUAL_SERIALIZE( information);
                 CASUAL_SERIALIZE( routes);
                 CASUAL_SERIALIZE( gtrids);
               )
            };
         } // extract

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
            struct
            {
               template< typename R>
               using fan_out = common::message::coordinate::fan::Out< R, common::strong::socket::id>;

               fan_out< casual::common::message::transaction::resource::prepare::Reply> prepare;
               fan_out< casual::common::message::transaction::resource::commit::Reply> commit;
               fan_out< casual::common::message::transaction::resource::rollback::Reply> rollback;

               inline void failed( common::strong::socket::id descriptor)
               {
                  prepare.failed( descriptor);
                  commit.failed( descriptor);
                  rollback.failed( descriptor);
               }

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( prepare);
                  CASUAL_SERIALIZE( commit);
                  CASUAL_SERIALIZE( rollback);
               )

            } transaction;

            common::message::coordinate::fan::Out< casual::domain::message::discovery::Reply, common::strong::socket::id> discovery;

            inline void failed( common::strong::socket::id descriptor)
            {
               transaction.failed( descriptor);
               discovery.failed( descriptor);
            }
            
            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( transaction);
               CASUAL_SERIALIZE( discovery);
            )
         } coordinate;

         //! holds all connections that has been requested to disconnect.
         std::vector< common::strong::socket::id> disconnecting;

         std::string alias;
         platform::size::type order{};

         //! `descriptor? has failed, @returns "all" extracted state associated with the `descriptor`
         state::extract::Result failed( common::strong::socket::id descriptor); 

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

            reply.state.pending.transactions = common::algorithm::transform( lookup.transactions(), []( auto& pair)
            {
               return message::outbound::state::pending::Transaction{
                  pair.first,
                  pair.second
               };
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
