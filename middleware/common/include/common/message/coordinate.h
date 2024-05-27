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

      namespace minimal::fan
      {
         namespace out
         {
            enum struct Directive
            {
               pending,
               done,
            };

            inline constexpr std::string_view description( Directive state) noexcept
            {
               switch( state)
               {
                  case Directive::pending: return "pending";
                  case Directive::done: return "done";
               }
               return "<unknown>";
            }

            struct Pending
            {
               enum struct State : short
               {
                  pending,
                  failed,
               };

               inline friend constexpr std::string_view description( State state) noexcept
               {
                  switch( state)
                  {
                     case State::pending: return "pending";
                     case State::failed: return "failed";
                  }
                  return "<unknown>";
               }

               Pending( strong::correlation::id correlation, process::Handle process)
                  : correlation{ correlation}, process{ process}
               {}

               State state = State::pending;
               strong::correlation::id correlation;
               process::Handle process;

               inline friend bool operator == ( const Pending& lhs, const strong::correlation::id& rhs) { return lhs.correlation == rhs;}
               inline friend bool operator == ( const Pending& lhs, process::compare_equal_to_handle auto rhs) { return lhs.process == rhs;}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( state);
                  CASUAL_SERIALIZE( correlation);
                  CASUAL_SERIALIZE( process);
               )

            };

            template< typename message_type>
            struct Entry
            {
               using pendings_type = std::vector< Pending>;
               using message_callback_type = common::unique_function< Directive( const message_type&)>;
               using done_callback_type = common::unique_function< void( pendings_type)>;

               Entry( std::vector< Pending> pending, message_callback_type message_callback, done_callback_type done_callback)
                  : m_pending{ std::move( pending)}, m_message_callback{ std::move( message_callback)}, m_done_callback{ std::move( done_callback)}
               {}

               Directive operator() ( const message_type& message)
               {
                  auto found = algorithm::find( m_pending, message.correlation);
                  CASUAL_ASSERT( found);
                  algorithm::container::erase( m_pending, std::begin( found));

                  if( m_message_callback( message) == Directive::done)
                     return Directive::done;

                  return is_done() ? Directive::done : Directive::pending;
               }

               Directive failed( process::compare_equal_to_handle auto process)
               {
                  for( auto& pending : m_pending)
                     if( pending == process)
                        pending.state = Pending::State::failed;

                  return is_done() ? Directive::done : Directive::pending;
               }

               bool is_done()
               {
                  if( ! algorithm::all_of( m_pending, []( auto& pending){ return pending.state == Pending::State::failed;}))
                     return false;

                  m_done_callback( std::move( m_pending));
                  return true;
               }


               inline friend bool operator == ( const Entry& lhs, const strong::correlation::id& rhs) { return algorithm::contains( lhs.m_pending, rhs);}

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( m_pending);
               )

            private:
               std::vector< Pending> m_pending;
               message_callback_type m_message_callback;
               done_callback_type m_done_callback;
            };
         } // out

         //! short-circuit fan-out. Entries them self dictates when they're done.
         //! this enables fan-out to be done before all replies has been received.
         template< typename message_type>
         struct Out
         {
            using entry_type = out::Entry< message_type>;
            using pendings_type = typename entry_type::pendings_type;

            void add( entry_type entry)
            {
               if( ! entry.is_done())
                  m_entries.push_back( std::move( entry));
            } 

            template< typename MC, typename DC>
            void add( pendings_type pendings, MC message_callback, DC done_callback) requires std::constructible_from< entry_type, pendings_type, MC, DC>
            {
               add( entry_type{ std::move( pendings), std::move( message_callback), std::move( done_callback)});
            } 

            void operator() ( message_type message)
            {
               if( auto found = algorithm::find( m_entries, message.correlation))
                  if( (*found)( std::move( message)) == out::Directive::done)
                     algorithm::container::erase( m_entries, std::begin( found));
            }

            void failed( process::compare_equal_to_handle auto process)
            {
               algorithm::container::erase_if( m_entries, [ process]( auto& entry)
               {
                  return entry.failed( process) == out::Directive::done;
               });
            }


            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( m_entries);
            )

            auto empty() const noexcept { return m_entries.empty();}
            auto size() const noexcept { return m_entries.size();}

         private:

            std::vector< entry_type> m_entries;
         };
         
      } // minimal::fan


   } //common::message::coordinate
} // casual
