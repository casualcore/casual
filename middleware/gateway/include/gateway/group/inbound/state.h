//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/tcp.h"
#include "gateway/message.h"

#include "casual/task/concurrent.h"

#include "common/serialize/macro.h"
#include "common/communication/select.h"
#include "common/communication/tcp.h"
#include "common/communication/ipc.h"
#include "common/communication/ipc/send.h"
#include "common/domain.h"
#include "common/message/service.h"
#include "common/state/machine.h"

#include "configuration/model.h"

#include <string>
#include <vector>

namespace casual
{
   namespace gateway::group::inbound
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

         using Limit = configuration::model::gateway::inbound::Limit;


         struct Correlation
         {
            Correlation() = default;
            Correlation( common::strong::correlation::id correlation, common::strong::socket::id descriptor)
               : correlation{ std::move( correlation)}, descriptor{ descriptor} {}

            common::strong::correlation::id correlation;
            common::strong::socket::id descriptor;

            inline friend bool operator == ( const Correlation& lhs, const common::strong::correlation::id& rhs) { return lhs.correlation == rhs;}
            inline friend bool operator == ( const Correlation& lhs, common::strong::socket::id rhs) { return lhs.descriptor == rhs;} 

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( correlation);
               CASUAL_SERIALIZE( descriptor);
            )
         };

         struct Conversation
         {
            common::strong::correlation::id correlation;
            common::strong::socket::id descriptor;
            common::process::Handle process;

            inline friend bool operator == ( const Conversation& lhs, const common::strong::correlation::id& rhs) { return lhs.correlation == rhs;}

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( correlation);
               CASUAL_SERIALIZE( descriptor);
               CASUAL_SERIALIZE( process);
            )
         };

         namespace extract
         {
            struct Result
            {
               group::tcp::External< configuration::model::gateway::inbound::Connection>::Information information;
               std::vector< common::strong::correlation::id> correlations; 
               std::vector< common::transaction::ID> trids;

               inline bool empty() const noexcept { return correlations.empty() && trids.empty();}

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( information);
                  CASUAL_SERIALIZE( correlations);
                  CASUAL_SERIALIZE( trids);
               )
            };
         } // extract

         namespace transaction
         {
            struct Cache
            {
               //! associate the external trid with the `descriptor`. @returns a cached branched internal trid, if any.
               const common::transaction::ID* associate( const common::transaction::ID& external, common::strong::socket::id descriptor);

               void add( const common::transaction::ID& branched_trid, common::strong::socket::id descriptor);

               //! @returns true if the descriptor is associated with any trids
               bool associated( common::strong::socket::id descriptor) const noexcept;

               const common::transaction::ID* find( common::transaction::global::id::range gtrid) const noexcept;

               //! extract all trids that will be 'lost' due to the failed `descriptor`.
               std::vector< common::transaction::ID> failed( common::strong::socket::id descriptor) noexcept;
               
               //! remove the `gtrid` and it's state
               void remove( common::transaction::global::id::range gtrid);

               inline bool empty() const noexcept { return m_transactions.empty();}

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( m_transactions);
               )

            private:

               struct Association
               {
                  inline Association( common::strong::socket::id descriptor, const common::transaction::ID& trid)
                     : trid{ trid}, descriptors{ descriptor} {}

                  common::transaction::ID trid;
                  std::vector< common::strong::socket::id> descriptors;

                  inline friend bool operator == ( const Association& lhs, common::strong::socket::id rhs) { return common::algorithm::contains( lhs.descriptors, rhs);}

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( trid);
                     CASUAL_SERIALIZE( descriptors);
                  )
               };

               std::unordered_map< common::transaction::global::ID, Association, common::transaction::global::hash, std::equal_to<>> m_transactions;
            };
            
         } // transaction

      } // state

      struct State
      {
         common::state::Machine< state::Runlevel, state::Runlevel::running> runlevel;

         common::communication::select::Directive directive;
         common::communication::ipc::send::Coordinator multiplex{ directive};

         group::tcp::External< configuration::model::gateway::inbound::Connection> external;

         casual::task::concurrent::Coordinator< common::strong::socket::id> tasks;

         struct
         {
            std::vector< common::strong::socket::id> disconnects;
            
            CASUAL_LOG_SERIALIZE( 
               //CASUAL_SERIALIZE( requests);
               CASUAL_SERIALIZE( disconnects);
            )
         } pending;
         

         state::transaction::Cache transaction_cache;

         state::Limit limit;

         std::string alias;
         std::string note;


         //! @returns true if `descriptor` is ready to be disconnected.
         bool disconnectable( common::strong::socket::id descriptor) const noexcept;
         
         //! @return true if the state is ready to 'terminate'
         bool done() const noexcept;


         state::extract::Result extract( common::strong::socket::id connection);

         //! @returns a reply message to state `request` that is filled with what's possible
         template< typename M>
         auto reply( M&& request) const noexcept
         {
            auto reply = external.reply( request);

            reply.state.alias = alias;
            reply.state.note = note;
            reply.state.limit = limit;

            return reply;
         }

         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( external);
            CASUAL_SERIALIZE( pending);
            //CASUAL_SERIALIZE( correlations);
            //CASUAL_SERIALIZE( conversations);
            CASUAL_SERIALIZE( transaction_cache);
            CASUAL_SERIALIZE( limit);
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( note);
         )
      };

   } // gateway::group::inbound
} // casual
