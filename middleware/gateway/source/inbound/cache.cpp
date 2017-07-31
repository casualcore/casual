//!
//! casual 
//!

#include "gateway/inbound/cache.h"
#include "gateway/common.h"

#include "common/algorithm.h"

namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace inbound
      {


         Cache::Limit::Limit() = default;
         Cache::Limit::Limit( size_type size, size_type messages) : size( size), messages( messages) {}


         Cache::Cache() = default;

         Cache::Cache( Limit limit) : m_limit{ std::move( limit)}
         {

         }

         Cache::~Cache()
         {
            terminate();
         }

         void Cache::set( Limit limit) const
         {
            lock_type lock{ m_mutex};

            m_limit = std::move( limit);

            //
            // We don't fiddle with state, and let it take it's natural course.
            //

         }

         void Cache::terminate() const
         {
            lock_type lock{ m_mutex};

            m_state = State::terminate;
            lock.unlock();
            m_condition.notify_all();
         }

         void Cache::add( complete_type&& message) const
         {
            Trace trace{ "gateway::inbound::Cache::add"};

            lock_type lock{ m_mutex};

            m_condition.wait( lock, [&]{ return m_state != State::limit;});

            if( m_state == State::terminate)
            {
               throw exception::Shutdown{ "conditional variable wants to shutdown..."};
            }

            m_size += message.payload.size();
            m_messages.push_back( std::move( message));

            //log << "cache: " << *this << '\n';

            if( ! vacant( lock))
            {
               log << "state change: vacant -> limit\n";

               //
               // This add to the cache broke the limits.
               //
               m_state = State::limit;
            }
         }

         Cache::complete_type Cache::get( const common::Uuid& correlation) const
         {
            Trace trace{ "gateway::inbound::Cache::get"};

            lock_type lock{ m_mutex};
            auto found = range::find( m_messages, correlation);

            if( ! found)
            {
               throw common::exception::invalid::Argument{ "failed to find correlation - Cache::get", CASUAL_NIP( correlation)};
            }

            auto result = std::move( *found);
            m_messages.erase( std::begin( found));
            m_size -= result.payload.size();

            //log << "cache: " << *this << '\n';

            if( m_state == State::limit && vacant( lock))
            {
               log << "state change: limit -> vacant\n";
               //
               // We was in a limit state, now we're vacant. Make sure blocked threads are
               // notified
               //
               m_state = State::vacant;

               //
               // Manual unlocking is done before notifying, to avoid waking up
               // the waiting thread only to block again (see notify_one for details)
               //
               lock.unlock();
               m_condition.notify_one();
            }

            return result;
         }

         bool Cache::vacant( const lock_type&) const
         {
            if( m_limit.messages && common::range::size( m_messages) >= m_limit.messages)
            {
               return false;
            }
            if( m_limit.size && m_size >= m_limit.size)
            {
               return false;
            }
            return true;
         }

         Cache::Limit Cache::size() const
         {
            lock_type lock{ m_mutex};
            return { m_size, common::range::size( m_messages)};
         }

         std::ostream& operator << ( std::ostream& out, const Cache& value)
         {
            auto get_state = []( Cache::State state){
               switch( state)
               {
                  case Cache::State::limit: return "limit";
                  case Cache::State::terminate: return "terminate";
                  case Cache::State::vacant: return "vacant";
               }
               return "?";
            };

            return out << "{ size: " << value.m_size
                  << ", messages: " << value.m_messages.size()
                  << ", state: " << get_state( value.m_state)
                  << ", limit: { messages: " << value.m_limit.messages << ", size: " << value.m_limit.size << '}'
                  << '}';
         }


      } // inbound
   } // gateway




} // casual
