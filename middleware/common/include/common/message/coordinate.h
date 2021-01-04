//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/functional.h"

#include "common/uuid.h"
#include "common/algorithm.h"
#include "common/serialize/macro.h"

#include <vector>

namespace casual
{
   namespace common::message::coordinate
   {
      namespace fan
      {

         template< typename M, typename ID>
         struct Out
         {
            using message_type = M;
            using id_type = ID;

            //! type to correlate the pending fan out request with the upcoming replies
            struct Pending
            {
               inline Pending( common::Uuid correlation, id_type id)
                  : correlation{ std::move( correlation)}, id{ id} {}

               common::Uuid correlation;
               id_type id;

               inline friend bool operator == ( const Pending& lhs, const common::Uuid& rhs) { return lhs.correlation == rhs;}
               inline friend bool operator == ( const Pending& lhs, id_type rhs) { return lhs.id == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( correlation);
                  CASUAL_SERIALIZE( id);
               )
            };

            Out() = default;
            Out( Out&&) noexcept = default;
            Out& operator = ( Out&&) noexcept = default;

            //! register pending 'fan outs' and a callback which is invoked when all pending
            //! has been 'received'.
            template< typename C>
            auto operator () ( std::vector< Pending> pending, C&& callback)
               -> decltype( void( callback( std::vector< message_type>{}, std::vector< id_type>{})))
            {
               auto& entry = m_entries.emplace_back( std::move( pending), std::forward< C>( callback));

               // to make it symmetrical and 'impossible' to add 'dead letters'.
               if( entry.done())
                  m_entries.pop_back();
            }

            //! 'pipe' the `message` to the 'fan-out-coordination'. Will invoke callback if `message` is
            //! the last pending message for an entry.
            inline void operator () ( message_type message)
            {
               if( auto found = algorithm::find( m_entries, message.correlation))
                  if( found->coordinate( std::move( message)))
                     m_entries.erase( std::begin( found));
            }

            inline void failed( id_type id)
            {
               algorithm::trim( m_entries, algorithm::remove_if( m_entries, [id]( auto& message)
               {
                  return message.failed( id);
               }));
            }

            inline auto empty() const noexcept { return m_entries.empty();}

            //! @returns an empty 'pending_type' vector
            //! convince function to get 'the right type' 
            inline auto empty_pendings() { return std::vector< Pending>{};}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_entries, "entries");
            )

         private:

            struct Entry
            {
               using callback_t = common::function< void( std::vector< message_type> received, std::vector< id_type> failed)>;

               inline Entry( std::vector< Pending> pending, callback_t callback)
                  : m_pending{ std::move( pending)}, m_callback{ std::move( callback)} {}

               inline bool coordinate( message_type message)
               {
                  algorithm::trim( m_pending, algorithm::remove( m_pending, message.correlation));

                  m_received.push_back( std::move( message));

                  return done();
               }

               inline bool failed( id_type id)
               {
                  auto [ keep, removed] = algorithm::partition( m_pending, predicate::negate( predicate::value::equal( id)));
                  if( removed)
                  {
                     m_failed.push_back( id);
                     algorithm::trim( m_pending, keep);
                  }
                  return done();
               }

               inline friend bool operator == ( const Entry & lhs, const common::Uuid& rhs) 
               { 
                  return ! algorithm::find( lhs.m_pending, rhs).empty();
               }

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( m_pending, "pending");
                  CASUAL_SERIALIZE_NAME( m_received, "received");
                  CASUAL_SERIALIZE_NAME( m_failed, "failed");
               )


               bool done()
               {
                  if( ! m_pending.empty())
                     return false;

                  log::line( verbose::log, "entry: ", *this);

                  m_callback( std::move( m_received), std::move( m_failed));
                  return true;
               }

            private:
               std::vector< Pending> m_pending;
               std::vector< message_type> m_received;
               std::vector< id_type> m_failed;
               callback_t m_callback;
               
            };

            std::vector< Entry> m_entries;
         };            
      } // fan

   } //common::message::coordinate
} // casual
