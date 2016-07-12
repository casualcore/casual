//!
//! casual 
//!

#include "gateway/inbound/cache.h"
#include "gateway/common.h"

#include "common/algorithm.h"
#include "common/trace.h"

namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace inbound
      {


         Cache::Limit::Limit() = default;
         Cache::Limit::Limit( std::size_t size, std::size_t messages) : size( size), messages( messages) {}


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

            m_messages.push_back( std::move( message));
            m_size += message.payload.size();

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
            if( m_limit.messages && m_messages.size() >= m_limit.messages)
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
            return { m_size, m_messages.size()};
         }


      } // inbound
   } // gateway




} // casual
