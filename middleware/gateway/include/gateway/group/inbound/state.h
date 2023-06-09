//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/tcp.h"
#include "gateway/message.h"

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

         namespace pending
         {
            using Limit = configuration::model::gateway::inbound::Limit;

            struct Requests
            {
               using complete_type = common::communication::ipc::message::Complete;

               inline pending::Limit limit() const { return m_limits;}
               inline void limit( pending::Limit limit) { m_limits = limit;}

               inline bool congested() const
               {
                  if( m_limits.size > 0 && m_size > m_limits.size)
                     return true;

                  return m_limits.messages > 0 && static_cast< platform::size::type>( m_services.size() + m_complete.size()) > m_limits.messages;
               }
               
               template< typename M>
               inline auto add( M&& message)
               {
                  static_assert( ! std::is_same_v< std::decay_t< M>, common::message::service::call::callee::Request>);
                  m_complete.push_back( common::serialize::native::complete< complete_type>( std::forward< M>( message)));
                  m_size += m_complete.back().size();
               }

               inline void add( common::message::service::call::callee::Request&& message)
               {
                  m_size += Requests::size( message);
                  m_services.push_back( std::move( message));
               }

               //! consumes pending calls, and sets the 'pending-roundtrip-state'
               complete_type consume( const common::strong::correlation::id& correlation, const common::message::service::lookup::Reply& lookup);
               complete_type consume( const common::strong::correlation::id& correlation);

               //! consumes all pending associated with the correlations, if any.
               struct Result
               {
                  std::vector< common::message::service::call::callee::Request> services;
                  std::vector< complete_type> complete;

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( services);
                     CASUAL_SERIALIZE( complete);
                  )
               };

               Result consume( const std::vector< common::strong::correlation::id>& correlations);

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE_NAME( m_services, "services");
                  CASUAL_SERIALIZE_NAME( m_complete, "complete");
                  CASUAL_SERIALIZE_NAME( m_size, "size");
                  CASUAL_SERIALIZE_NAME( m_limits, "limits");
               )

            private:

               inline static platform::size::type size( const common::message::service::call::callee::Request& message)
               {
                  return sizeof( message) + message.buffer.data.size() + message.buffer.type.size();
               }

               std::vector< common::message::service::call::callee::Request> m_services;
               std::vector< complete_type> m_complete;
               platform::size::type m_size = 0;
               pending::Limit m_limits;
               
            };

         } // pending


         struct Correlation
         {
            Correlation() = default;
            Correlation( common::strong::correlation::id correlation, common::strong::file::descriptor::id descriptor)
               : correlation{ std::move( correlation)}, descriptor{ descriptor} {}

            common::strong::correlation::id correlation;
            common::strong::file::descriptor::id descriptor;

            inline friend bool operator == ( const Correlation& lhs, const common::strong::correlation::id& rhs) { return lhs.correlation == rhs;}
            inline friend bool operator == ( const Correlation& lhs, common::strong::file::descriptor::id rhs) { return lhs.descriptor == rhs;} 

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( correlation);
               CASUAL_SERIALIZE( descriptor);
            )
         };

         struct Conversation
         {
            common::strong::correlation::id correlation;
            common::strong::file::descriptor::id descriptor;
            common::process::Handle process;

            inline friend bool operator == ( const Conversation& lhs, const common::strong::correlation::id& rhs) { return lhs.correlation == rhs;}

            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( correlation);
               CASUAL_SERIALIZE( descriptor);
               CASUAL_SERIALIZE( process);
            )
         };

         namespace in::flight
         {
            struct Cache
            {
               void add( common::strong::file::descriptor::id descriptor, const common::transaction::ID& trid);
               void remove( common::strong::file::descriptor::id descriptor, const common::transaction::ID& trid);

               //! remove all associated transactions to the `descriptor`
               void remove( common::strong::file::descriptor::id descriptor);

               bool empty( common::strong::file::descriptor::id descriptor) const noexcept;

               
               
               //! @returns and extract/remove the associated transactions tor the `descriptor`. 
               std::vector< common::transaction::ID> extract( common::strong::file::descriptor::id descriptor);

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( m_transactions);
               )

            private:
               std::unordered_map< common::strong::file::descriptor::id, std::vector< common::transaction::ID>> m_transactions;
            };
         } // in::flight

         namespace extract
         {
            struct Result
            {
               group::tcp::External< configuration::model::gateway::inbound::Connection>::Information information;
               state::pending::Requests::Result pending;
               std::vector< common::transaction::ID> transactions;

               inline bool empty() const noexcept { return pending.complete.empty() && pending.services.empty() && transactions.empty();}

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( information);
                  CASUAL_SERIALIZE( pending);
                  CASUAL_SERIALIZE( transactions);
               )

            };
         } // extract

      } // state

      struct State
      {
         common::state::Machine< state::Runlevel, state::Runlevel::running> runlevel;

         common::communication::select::Directive directive;
         group::tcp::External< configuration::model::gateway::inbound::Connection> external;

         struct
         {
            state::pending::Requests requests;
            std::vector< common::strong::file::descriptor::id> disconnects;
            
            CASUAL_LOG_SERIALIZE( 
               CASUAL_SERIALIZE( requests);
               CASUAL_SERIALIZE( disconnects);
            )
         } pending;

         common::communication::ipc::send::Coordinator multiplex{ directive};
         
         std::vector< state::Correlation> correlations;
         std::vector< state::Conversation> conversations;

         state::in::flight::Cache in_flight_cache;

         std::string alias;
         std::string note;


         //! @return the correlated descriptor, and remove the correlation
         common::strong::file::descriptor::id consume( const common::strong::correlation::id& correlation);

         tcp::Connection* connection( const common::strong::correlation::id& correlation);

         //! @returns true if `descriptor` is ready to be disconnected.
         bool disconnectable( common::strong::file::descriptor::id descriptor) const noexcept;
         
         //! @return true if the state is ready to 'terminate'
         bool done() const noexcept;

         state::extract::Result extract( common::strong::file::descriptor::id connection);

         //! @returns a reply message to state `request` that is filled with what's possible
         template< typename M>
         auto reply( M&& request) const noexcept
         {
            auto reply = external.reply( request);

            reply.state.alias = alias;
            reply.state.note = note;
            reply.state.limit = pending.requests.limit();

            return reply;
         }

         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( external);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( correlations);
            CASUAL_SERIALIZE( conversations);
            CASUAL_SERIALIZE( in_flight_cache);
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( note);
         )
      };

   } // gateway::group::inbound
} // casual
