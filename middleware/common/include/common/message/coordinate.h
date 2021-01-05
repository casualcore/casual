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
               enum struct State : short
               {
                  pending,
                  received,
                  failed,
               };

               inline friend std::ostream& operator << ( std::ostream& out, State state)
               {
                  switch( state)
                  {
                     case State::pending: return out << "pending";
                     case State::received: return out << "received";
                     case State::failed: return out << "failed";
                  }
                  return out << "<unknown>";
               }

               Pending() = default;
               inline Pending( common::Uuid correlation, id_type id)
                  : id{ id}, correlation{ std::move( correlation)}{}

               State state = State::pending;
               id_type id;
               common::Uuid correlation;
               
               inline friend bool operator == ( const Pending& lhs, const common::Uuid& rhs) { return lhs.correlation == rhs;}
               inline friend bool operator == ( const Pending& lhs, id_type rhs) { return lhs.id == rhs;}
               inline friend bool operator == ( const Pending& lhs, State rhs) { return lhs.state == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( state);
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( correlation);
               )
            };

            Out() = default;
            Out( Out&&) noexcept = default;
            Out& operator = ( Out&&) noexcept = default;

            //! register pending 'fan outs' and a callback which is invoked when all pending
            //! has been 'received'.
            template< typename C>
            auto operator () ( std::vector< Pending> pending, C&& callback)
               -> decltype( void( callback( std::vector< message_type>{}, std::vector< Pending>{})))
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
               algorithm::trim( m_entries, algorithm::remove_if( m_entries, [id]( auto& entry)
               {
                  return entry.failed( id);
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
               using callback_t = common::function< void( std::vector< message_type> received, std::vector< Pending> outcome)>;

               inline Entry( std::vector< Pending> pending, callback_t callback)
                  : m_pending{ std::move( pending)}, m_callback{ std::move( callback)} {}

               inline bool coordinate( message_type message)
               {
                  if( auto found = algorithm::find( m_pending, message.correlation))
                     found->state = Pending::State::received;
                  
                  m_received.push_back( std::move( message));

                  return done();
               }

               inline bool failed( id_type id)
               {
                  auto is_pending = [id]( auto& pending){ return pending.id == id && pending.state == Pending::State::pending;};
                  
                  for( auto& pending : algorithm::filter( m_pending, is_pending))
                     pending.state = Pending::State::failed;

                  return done();
               }

               inline friend bool operator == ( const Entry & lhs, const common::Uuid& rhs) 
               { 
                  return ! algorithm::find( lhs.m_pending, rhs).empty();
               }

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( m_pending, "pending");
                  CASUAL_SERIALIZE_NAME( m_received, "received");
               )


               bool done()
               {
                  if( algorithm::any_of( m_pending, predicate::value::equal( Pending::State::pending)))
                     return false;

                  log::line( verbose::log, "entry: ", *this);

                  m_callback( std::move( m_received), std::move( m_pending));
                  return true;
               }

            private:
               std::vector< Pending> m_pending;
               std::vector< message_type> m_received;
               callback_t m_callback;
               
            };

            std::vector< Entry> m_entries;
         };            
      } // fan

   } //common::message::coordinate
} // casual
