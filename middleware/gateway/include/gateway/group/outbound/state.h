//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/connection.h"
#include "gateway/group/tcp.h"
#include "gateway/message.h"

#include "casual/task/concurrent.h"

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
      struct State;

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
         using task_coordinator_type = task::concurrent::Coordinator< common::strong::socket::id>;

         enum struct Runlevel : short
         {
            running,
            shutdown,
            error,
         };
         std::string_view description( Runlevel value);


        namespace service
        {
            struct Metric
            {
               void add( common::message::event::service::Metric&& metric)
               {
                  m_metric.metrics.push_back( std::move( metric));
               }

               //! "force" metric callback if there are metric to send
               template< typename CB>
               void force_metric( State& state, CB callback)
               {
                  if( m_metric.metrics.empty())
                     return;

                  callback( state, m_metric);
                  m_metric.metrics.clear();
               }

               template< typename CB>
               void maybe_metric( State& state, CB callback)
               {
                  if( m_metric.metrics.size() >= platform::batch::gateway::metrics)
                     force_metric( state, callback);
               }

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( m_metric);
               )

            private:
               common::message::event::service::Calls m_metric{ common::process::handle()};
            };
        } // service

         namespace pending
         {
            struct Transactions
            {
               bool associate( const common::transaction::ID& trid, common::strong::socket::id descriptor);
               void remove( const common::transaction::ID& trid, common::strong::socket::id descriptor);
               void remove( common::transaction::global::id::range gtrid);

               bool is_associated( const common::transaction::ID& trid, common::strong::socket::id descriptor);

               std::vector< common::transaction::global::ID> extract( common::strong::socket::id descriptor);

               bool contains( common::strong::socket::id descriptor) const noexcept;

               const auto& transactions() const noexcept { return m_transactions;}

               inline bool empty() const noexcept { return m_transactions.empty();}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( m_transactions);
               )

            private:
               std::unordered_map< common::transaction::global::ID, std::vector< common::strong::socket::id>, common::transaction::global::hash, std::equal_to<>> m_transactions;
            };
            
         } // pending



         namespace reply
         {
            namespace destination
            {
               struct Entry
               {
                  common::strong::correlation::id correlation;
                  common::strong::ipc::id ipc;
                  common::strong::socket::id connection;
                  common::message::Type type;

                  inline friend bool operator == ( const Entry& lhs, const common::strong::correlation::id& rhs) noexcept { return lhs.correlation == rhs;}
                  inline friend bool operator == ( const Entry& lhs, common::strong::socket::id rhs) noexcept { return lhs.connection == rhs;}

                  inline explicit operator bool () const noexcept { return common::predicate::boolean( ipc);}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( correlation);
                     CASUAL_SERIALIZE( ipc);
                  )
               };
               
            } // destination


            struct Destination
            {
               //! @returns false if correlation is found in _entries_, otherwise adds the entry and returns true.
               template< typename M>
               bool add( const M& message, common::strong::socket::id connection)
               {
                  if( common::algorithm::find( m_entries, message.correlation))
                     return false;
                  
                  m_entries.push_back( destination::Entry{ message.correlation, message.process.ipc, connection, message.type()});
                  return true;
               }

               destination::Entry extract( const common::strong::correlation::id& correlation)
               {
                  if( auto found = common::algorithm::find( m_entries, correlation))
                     return common::algorithm::container::extract( m_entries, std::begin( found));

                  return {};
               }

               std::vector< destination::Entry> extract( common::strong::socket::id connection)
               {
                  if( auto range = common::algorithm::filter( m_entries, common::predicate::value::equal( connection)))
                     return common::algorithm::container::extract( m_entries, range);

                  return {};
               }

               inline bool contains( const common::strong::correlation::id& correlation) const noexcept 
               { 
                  return common::algorithm::contains( m_entries, correlation);
               }

               inline bool contains( common::strong::socket::id connection) const noexcept 
               { 
                  return common::algorithm::contains( m_entries, connection);
               }

               inline const auto& entries() const noexcept { return m_entries;}
               inline bool empty() const noexcept { return m_entries.empty();}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( m_entries);
               )

            private:
               std::vector< destination::Entry> m_entries;
            };

         } // reply

         namespace extract
         {
            struct Result
            {
               group::tcp::connection::Information< casual::configuration::model::gateway::outbound::Connection> information;
               std::vector< reply::destination::Entry> destinations;
               std::vector< common::transaction::global::ID> gtrids;

               inline bool empty() const noexcept { return destinations.empty() && gtrids.empty();}

               CASUAL_LOG_SERIALIZE( 
                 CASUAL_SERIALIZE( information);
                 CASUAL_SERIALIZE( destinations);
                 CASUAL_SERIALIZE( gtrids);
               )
            };
         } // extract

         namespace disconnect
         {
            enum struct Directive : short
            {
               disconnect,
               remove,
            };
            std::string_view description( Directive value);
            
         } // disconnect

         struct Disconnect
         {
            common::strong::socket::id descriptor;
            disconnect::Directive directive;
            
            inline friend bool operator == ( const Disconnect& lhs, common::strong::socket::id rhs) { return lhs.descriptor == rhs;}

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( directive);
               CASUAL_SERIALIZE( descriptor);
            )
         };

      } // state

      struct State
      {
         common::state::Machine< state::Runlevel, state::Runlevel::running> runlevel;
         
         common::communication::select::Directive directive;
         common::communication::ipc::send::Coordinator multiplex{ directive};

         group::connection::Holder< casual::configuration::model::gateway::outbound::Connection> connections;
         std::vector< casual::configuration::model::gateway::outbound::Connection> disabled_connections;

         state::task_coordinator_type tasks;
         
         state::reply::Destination reply_destination;
         state::service::Metric service_metric;

         
         struct
         {
            state::pending::Transactions transactions;

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( transactions);
            )

         } pending;


         //! holds all connections that has been requested to disconnect.
         std::vector< state::Disconnect> disconnecting;

         std::string alias;
         platform::size::type order{};

         //! `descriptor? has failed, @returns "all" extracted state associated with the `descriptor`
         state::extract::Result failed( common::strong::socket::id descriptor); 

         //! @returns true if the `descriptor` does not have any thing "in-flight"
         bool idle( common::strong::socket::id descriptor) const noexcept;

         bool done() const;

         //! @returns a reply message to state `request` that is filled with what's possible
         template< typename M>
         auto reply( M&& request) const noexcept
         {
            auto reply = connections.reply( request);

            // update disconnect informaiton, if any
            for( auto& disconnect : disconnecting)
               if( auto found = common::algorithm::find( reply.state.connections, disconnect.descriptor))
                  found->runlevel = decltype( found->runlevel)::disconnecting;

            reply.state.alias = alias;
            reply.state.order = order;

            // pending
            reply.state.pending.tasks = common::algorithm::transform( tasks.tasks(), []( auto& task)
            {
               message::outbound::state::pending::Task result;
               result.correlation = task.correlation();
               result.connection = task.key();
               result.message_types = task.types();
               return result;
            });

            reply.state.pending.transactions = common::algorithm::transform( pending.transactions.transactions(), []( auto& pair)
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
            CASUAL_SERIALIZE( connections);
            CASUAL_SERIALIZE( reply_destination);
            CASUAL_SERIALIZE( tasks);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( service_metric);
            CASUAL_SERIALIZE( disconnecting);
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( order);
         )
      };

   } // gateway::group::outbound
} // casual
