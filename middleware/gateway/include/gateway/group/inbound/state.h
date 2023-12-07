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

/*
         namespace pending
         {
            

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
               inline auto add( const M& message)
               {
                  static_assert( ! std::is_same_v< std::decay_t< M>, common::message::service::call::callee::Request>);
                  m_size += m_complete.emplace_back( message).complete.size();
               }

               inline void add( common::message::service::call::callee::Request&& message)
               {
                  m_size += Requests::size( message);
                  m_services.push_back( std::move( message));
               }

               //! consumes pending calls, and sets the 'pending-roundtrip-state'
               std::optional< complete_type> consume( const common::strong::correlation::id& correlation, const common::message::service::lookup::Reply& lookup);
               std::optional< complete_type> consume( const common::strong::correlation::id& correlation);

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

               //! @returns "error replies" if there are pending messages associated with the gtrid that is to be rolled back
               //! @attention this is a hack that only fixes part of premature rollback due to timeout. 
               //! TODO fix and remove in 1.7
               std::vector< common::communication::tcp::message::Complete> abort_pending( const common::message::transaction::resource::rollback::Request& message);

               Result consume( const std::vector< common::strong::correlation::id>& correlations);

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE_NAME( m_services, "services");
                  CASUAL_SERIALIZE_NAME( m_complete, "complete");
                  CASUAL_SERIALIZE_NAME( m_size, "size");
                  CASUAL_SERIALIZE_NAME( m_limits, "limits");
               )

            private:

               struct Holder
               {
                  template< typename M>
                  Holder( const M& message) 
                     : gtrid{ common::transaction::id::range::global( message.trid)}, complete{ common::serialize::native::complete< complete_type>( message)} 
                  {
                     using Type = common::message::Type;

                     // make sure we only add expected types
                     static_assert( common::algorithm::compare::any( M::type(), 
                        Type::conversation_connect_request,
                        Type::queue_group_dequeue_request,
                        Type::queue_group_enqueue_request));
                  }

                  common::transaction::global::ID gtrid;
                  complete_type complete;

                  inline friend bool operator == ( const Holder& lhs, const common::strong::correlation::id& rhs) { return lhs.complete.correlation() == rhs;}
                  inline friend bool operator == ( const common::strong::correlation::id& lhs, const Holder& rhs) { return lhs == rhs.complete.correlation();}

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( gtrid);
                     CASUAL_SERIALIZE( complete);
                  )
               };

               inline static platform::size::type size( const common::message::service::call::callee::Request& message)
               {
                  return sizeof( message) + message.buffer.data.size() + message.buffer.type.size();
               }

               std::vector< common::message::service::call::callee::Request> m_services;
               std::vector< Holder> m_complete;
               platform::size::type m_size = 0;
               pending::Limit m_limits;
               
            };

         } // pending
         */


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
               const common::transaction::ID* associate( common::strong::file::descriptor::id descriptor, const common::transaction::ID& external);
               void dissociate( common::strong::file::descriptor::id descriptor, const common::transaction::ID& external);

               void add_branch( common::strong::file::descriptor::id descriptor, const common::transaction::ID& branched_trid);

               //! @returns true if the descriptor is associated with 
               bool associated( common::strong::file::descriptor::id descriptor) const noexcept;

               const common::transaction::ID* find( common::transaction::global::id::range gtrid) const noexcept;

               std::vector< common::transaction::ID> extract( common::strong::file::descriptor::id descriptor) noexcept;

               inline bool empty() const noexcept { return m_associations.empty() && m_transactions.empty();}

               CASUAL_LOG_SERIALIZE( 
                  CASUAL_SERIALIZE( m_transactions);
                  CASUAL_SERIALIZE( m_associations);
               )

            private:

               struct Association
               {
                  inline Association( common::strong::file::descriptor::id descriptor, const common::transaction::ID& trid)
                     : descriptor{ descriptor}, trids{ trid} {}

                  common::strong::file::descriptor::id descriptor;
                  std::unordered_set< common::transaction::ID> trids;

                  inline friend bool operator == ( const Association& lhs, common::strong::file::descriptor::id rhs) { return lhs.descriptor == rhs;}

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( descriptor);
                     CASUAL_SERIALIZE( trids);
                  )
               };

               struct Map
               {
                  inline Map( common::strong::file::descriptor::id descriptor, const common::transaction::ID& trid)
                     : trid{ trid}, descriptors{ descriptor} {}

                  common::transaction::ID trid;
                  std::vector< common::strong::file::descriptor::id> descriptors;

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( trid);
                     CASUAL_SERIALIZE( descriptors);
                  )
               };

               std::vector< Association> m_associations;
               std::unordered_map< common::transaction::global::ID, Map, common::transaction::global::hash, std::equal_to<>> m_transactions;
            };
            
         } // transaction

      } // state

      struct State
      {
         common::state::Machine< state::Runlevel, state::Runlevel::running> runlevel;

         common::communication::select::Directive directive;
         common::communication::ipc::send::Coordinator multiplex{ directive};

         group::tcp::External< configuration::model::gateway::inbound::Connection> external;

         casual::task::concurrent::Coordinator tasks;

         struct
         {
            //state::pending::Requests requests;
            std::vector< common::strong::file::descriptor::id> disconnects;
            
            CASUAL_LOG_SERIALIZE( 
               //CASUAL_SERIALIZE( requests);
               CASUAL_SERIALIZE( disconnects);
            )
         } pending;
         
         std::vector< state::Correlation> correlations;
         std::vector< state::Conversation> conversations;

         state::transaction::Cache transaction_cache;

         state::Limit limit;

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
            reply.state.limit = limit;

            return reply;
         }

         CASUAL_LOG_SERIALIZE( 
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( external);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( correlations);
            CASUAL_SERIALIZE( conversations);
            CASUAL_SERIALIZE( transaction_cache);
            CASUAL_SERIALIZE( limit);
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( note);
         )
      };

   } // gateway::group::inbound
} // casual
