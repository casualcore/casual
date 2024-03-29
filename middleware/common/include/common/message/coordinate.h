//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/functional.h"

#include "common/uuid.h"
#include "common/log.h"
#include "common/algorithm.h"
#include "common/algorithm/sorted.h"
#include "common/algorithm/container.h"
#include "common/serialize/macro.h"

#include "casual/assert.h"

#include <vector>
#include <unordered_map>

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

               inline friend constexpr std::string_view description( State state) noexcept
               {
                  switch( state)
                  {
                     case State::pending: return "pending";
                     case State::received: return "received";
                     case State::failed: return "failed";
                  }
                  return "<unknown>";
               }

               Pending() = default;
               inline Pending( strong::correlation::id correlation, id_type id)
                  : id{ id}, correlation{ std::move( correlation)}{}

               State state = State::pending;
               id_type id;
               strong::correlation::id correlation;
               
               inline friend bool operator == ( const Pending& lhs, const strong::correlation::id& rhs) { return lhs.correlation == rhs;}
               inline friend bool operator == ( const Pending& lhs, State rhs) { return lhs.state == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( state);
                  CASUAL_SERIALIZE( id);
                  CASUAL_SERIALIZE( correlation);
               )
            };

            template< typename I>
            friend auto operator == ( const Pending& lhs, I&& rhs) noexcept -> decltype( lhs.id == rhs) { return lhs.id == rhs;}

            struct Entry
            {
               using callback_t = common::unique_function< void( std::vector< message_type> received, std::vector< Pending> outcome)>;

               inline Entry( std::vector< Pending> pending, callback_t callback)
                  : m_pending{ std::move( pending)}, m_callback{ std::move( callback)} {}

               inline auto coordinate( message_type message)
               {  
                  auto found = algorithm::find( m_pending, message.correlation);
                  CASUAL_ASSERT( found);

                  found->state = decltype( found->state)::received;
                  m_received.push_back( std::move( message));

                  return done();
               }

               //! Sets the pending state to failed regardless of previous state
               template< typename I>
               auto failed( I&& id) -> std::vector< strong::correlation::id>
               {
                  auto has_id = [ &id]( auto& pending){ return pending.id == id;};

                  return algorithm::transform_if( m_pending, []( auto& pending)
                  {
                     pending.state = Pending::State::failed; 
                     return pending.correlation;

                  }, has_id);
               }

               bool done()
               {
                  if( algorithm::any_of( m_pending, predicate::value::equal( Pending::State::pending)))
                     return false;

                  m_callback( std::move( m_received), std::move( m_pending));
                  return true;
               }


               inline auto& pending() const noexcept { return m_pending;}
               inline auto& received() const noexcept { return m_received;}

               constexpr auto type() const noexcept { return message_type::type();}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( m_pending, "pending");
                  CASUAL_SERIALIZE_NAME( m_received, "received");
               )

            private:
               std::vector< Pending> m_pending;
               std::vector< message_type> m_received;
               callback_t m_callback;
            };


            //! Register pending 'fan outs' and a callback which is invoked when all pending
            //! has been 'received'.
            //! @attention The callback will be invoked with all received messages and the total set of 
            //! requested/pending outcomes. `outcome` includes _received_ and _failed_, and its upp
            //! to the callback to decided what to do with it... 
            template< typename C>
            void operator () ( std::vector< Pending> pending, C&& callback)
            {
               auto entry = std::make_shared< Entry>( std::move( pending), std::forward< C>( callback));
               
               // to make it symmetrical and 'impossible' to add 'dead letters'.
               if( entry->done())
                  return;

               for( auto& pending : entry->pending())
                  m_lookup.emplace( pending.correlation, entry);
               
               m_entries.push_back( std::move( entry));
            }

            //! 'pipe' the `message` to the 'fan-out-coordination'. Will invoke callback if `message` is
            //! the last pending message for an entry.
            inline void operator () ( message_type message)
            {
               if( auto found = algorithm::find( m_lookup, message.correlation))
               {
                  // remove the entry if it's done.
                  if( found->second->coordinate( std::move( message)))
                     algorithm::container::erase( m_entries, found->second);

                  // always remove from lookup to keep the state as small as possible
                  m_lookup.erase( std::begin( found));
               }
            }

            //! Mark any pending as failed that are associated with `id`
            //! remove lookup correlation for the failed -> possible later "correlated" replies will be ignored
            //! This could trigger `done` and invocation of the callback -> cleanup of the entry.
            template< typename I>
            inline auto failed( I&& id) -> decltype( void( std::declval< const id_type&>() == id))
            {
               // this will potentially be slow...
               algorithm::container::erase_if( m_entries, [ &]( auto& entry)
               {
                  if( auto correlations = entry->failed( id); ! correlations.empty())
                  {
                     // remove all lookups for the entry, and return true to erase the entry it self
                     for( auto& correlation : correlations)
                         m_lookup.erase( correlation);

                     return entry->done();
                  }
                  return false;
               });
            }

            inline auto empty() const noexcept { return m_entries.empty() && m_lookup.empty();}

            using pending_type = std::vector< Pending>;

            //! @returns an empty 'pending_type' vector
            //! convenience function to get 'the right type' 
            inline auto empty_pendings() const noexcept { return pending_type{};}

            friend std::ostream& operator << ( std::ostream& out, const Out& value)
            {
               stream::write( out, "{ lookup: ", value.m_lookup, ", entries: [");
               // some manually print stuff to get the content of the shared_ptr to log...
               algorithm::for_each_interleave( 
                  value.m_entries,
                  [ &out]( auto& value){ stream::write( out, "{ address: ", value, ", content: ", *value, '}');},
                  [ &out](){ out << ", ";}
               );
               return out << "]}";
            }

         private:

            std::unordered_map< strong::correlation::id, std::shared_ptr< Entry>> m_lookup;
            std::vector< std::shared_ptr< Entry>> m_entries;
         };            
      } // fan

   } //common::message::coordinate
} // casual
