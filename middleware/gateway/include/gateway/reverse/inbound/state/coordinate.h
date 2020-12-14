//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/message.h"

#include "common/functional.h"

#include "common/uuid.h"
#include "common/algorithm.h"
#include "common/serialize/macro.h"

#include <vector>

namespace casual
{
   namespace gateway::reverse::inbound::state
   {
      namespace coordinate
      {
         namespace discovery
         {
            struct Message
            {
               struct Pending
               {
                  inline Pending( common::Uuid correlation, common::strong::process::id source)
                     : correlation{ correlation}, source{ source} {}

                  common::Uuid correlation;
                  common::strong::process::id source;

                  inline friend bool operator == ( const Pending& lhs, const common::Uuid& rhs) { return lhs.correlation == rhs;}
                  inline friend bool operator == ( const Pending& lhs, common::strong::process::id rhs) { return lhs.source == rhs;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( correlation);
                     CASUAL_SERIALIZE( source);
                  )
               };

               using callback_t = common::function< void( std::vector< common::message::gateway::domain::discover::Reply> received, std::vector< common::strong::process::id> failed)>;

               inline Message( std::vector< Pending> pending, callback_t callback)
                  : m_pending{ std::move( pending)}, m_callback{ std::move( callback)} {}

               inline bool coordinate( common::message::gateway::domain::discover::Reply message)
               {
                  common::algorithm::trim( m_pending, common::algorithm::remove( m_pending, message.correlation));

                  m_received.push_back( std::move( message));

                  return done();
               }

               inline bool failed( common::strong::process::id pid)
               {
                  if( auto found = common::algorithm::find( m_pending, pid))
                  {
                     m_pending.erase( std::begin( found));
                     m_failed.push_back( pid);
                  }
                  return done();
               }

               inline friend bool operator == ( const Message& lhs, const common::Uuid& rhs) 
               { 
                  return ! common::algorithm::find( lhs.m_pending, rhs).empty();
               }

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE_NAME( m_pending, "pending");
                  CASUAL_SERIALIZE_NAME( m_received, "received");
                  CASUAL_SERIALIZE_NAME( m_failed, "failed");
               )

            private:

               bool done()
               {
                  if( ! m_pending.empty())
                     return false;

                  m_callback( std::move( m_received), std::move( m_failed));
                  return true;
               }

               std::vector< Pending> m_pending;
               std::vector< common::message::gateway::domain::discover::Reply> m_received;
               std::vector< common::strong::process::id> m_failed;
               callback_t m_callback;
               
            };
         } // discovery

         struct Discovery
         {

            inline void operator () ( discovery::Message message)
            {
               m_messages.push_back( std::move( message));
            }

            template< typename C>
            auto operator () ( std::vector< discovery::Message::Pending> pending, C&& callback)
               -> decltype( void( discovery::Message{ pending, std::forward< C>( callback)}))
            {
               Discovery::operator () ( discovery::Message{ pending, std::forward< C>( callback)});
            }

            inline void operator () ( common::message::gateway::domain::discover::Reply message)
            {
               if( auto found = common::algorithm::find( m_messages, message.correlation))
               {
                  if( found->coordinate( std::move( message)))
                     m_messages.erase( std::begin( found));
               }  
            }

            inline void failed( common::strong::process::id pid)
            {
               common::algorithm::trim( m_messages, common::algorithm::remove_if( m_messages, [pid]( auto& message)
               {
                  return message.failed( pid);
               }));
            }

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_messages, "messages");
            )

         private:

            std::vector< discovery::Message> m_messages;
         };
         
      } // coordinate



   } // gateway::reverse::inbound::state
} // casual
