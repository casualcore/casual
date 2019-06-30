//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/process.h"
#include "common/stream.h"

#include <vector>

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace coordinate
         {
            namespace policy
            {
               struct Message
               {
                  inline Message( std::vector< strong::process::id> pids) : m_pids{ std::move( pids)} {}
                  inline Message( const std::vector< common::process::Handle>& processes)
                     : m_pids{ algorithm::transform( processes, []( const common::process::Handle& h){ return h.pid;})} {}

                  inline bool consume( strong::process::id pid)
                  {
                     auto found = algorithm::find( m_pids, pid);

                     if( ! found)
                        return false;
                     
                     m_pids.erase( std::begin( found));
                     return true;
                  }

                  inline bool done() const { return m_pids.empty();}

                  // for logging only
                  CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
                  {
                     CASUAL_SERIALIZE_NAME( m_pids, "pids");
                  })

               private:
                  std::vector< strong::process::id> m_pids;
               };

            } // policy
         } // coordinate

         template< typename Policy, typename MP = coordinate::policy::Message>
         struct Coordinate
         {
            using policy_type = Policy;
            using message_type = typename policy_type::message_type;
            using message_policy_type = MP;

            template< typename... Args>
            explicit Coordinate( Args&&... args) : m_policy{ std::forward< Args>( args)...} {}


            template< typename... Requested>
            void add( const Uuid& correlation, strong::ipc::id destination, Requested&&... requested)
            {
               m_messages.emplace_back(
                     destination,
                     correlation,
                     std::forward< Requested>( requested)...);


               if( m_messages.back().policy.done())
               {
                  m_policy.send( m_messages.back().ipc, m_messages.back().message);
                  m_messages.pop_back();
               }
            }

            template< typename Reply>
            bool accumulate( Reply&& message)
            {
               auto found = algorithm::find_if( m_messages, [&message]( const holder_type& m){
                  return m.message.correlation == message.correlation;
               });

               if( found && found->policy.consume( message.process.pid))
               {
                  // Accumulate message
                  m_policy.accumulate( found->message, std::forward< Reply>( message));

                  if( found->policy.done())
                  {
                     // We're done, send reply...
                     m_policy.send( found->ipc, found->message);
                     m_messages.erase( std::begin( found));
                  }

                  return true;
               }
               return false;
            }

            void remove( strong::process::id pid)
            {
               algorithm::trim( m_messages, algorithm::remove_if( m_messages, [=]( holder_type& h){
                  if( h.policy.consume( pid) && h.policy.done())
                  {
                     m_policy.send( h.ipc, h.message);
                     return true;
                  }
                  return false;
               }));
            }

            Policy& policy() { return m_policy;}
            const Policy& policy() const { return m_policy;}

            platform::size::type size() const { return m_messages.size();}

            // for logging only
            CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
            {
               CASUAL_SERIALIZE_NAME( m_messages, "messages");
               CASUAL_SERIALIZE_NAME( m_policy, "policy");
            })

         private:

            struct holder_type
            {
               template< typename... Args>
               holder_type( strong::ipc::id ipc, const Uuid& correlation, Args&&... args)
                  : ipc{ ipc}, policy{ std::forward< Args>( args)...}
               {
                  message.correlation = correlation;
               }

               bool done() const { return policy.done();}

               strong::ipc::id ipc;
               message_policy_type policy;
               message_type message;

               // for logging only
               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
               {
                  CASUAL_SERIALIZE( ipc);
                  CASUAL_SERIALIZE( policy);
                  CASUAL_SERIALIZE( message);
               })
            };


            std::vector< holder_type> m_messages;
            Policy m_policy;
         };


      } // message
   } // common
} // casual


