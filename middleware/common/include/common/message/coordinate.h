//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_COORDINATE_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_COORDINATE_H_


#include "common/uuid.h"
#include "common/exception.h"
#include "common/process.h"

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
                  inline Message( std::vector< platform::pid::type> pids) : m_pids{ std::move( pids)} {}
                  inline Message( const std::vector< common::process::Handle>& processes)
                     : m_pids{ range::transform( processes, []( const common::process::Handle& h){ return h.pid;})} {}

                  inline bool consume( platform::pid::type pid)
                  {
                     auto found = range::find( m_pids, pid);

                     if( found)
                     {
                        m_pids.erase( std::begin( found));
                        return true;
                     }
                     return false;

                  }

                  inline bool done() const
                  {
                     return m_pids.empty();
                  }

               private:
                  std::vector< platform::pid::type> m_pids;
               };

            } // policy

         } // coordinate

         template< typename Message, typename Policy, typename MP = coordinate::policy::Message>
         struct Coordinate
         {
            using policy_type = Policy;
            using message_policy_type = MP;

            template< typename... Args>
            Coordinate( Args&&... args) : m_policy{ std::forward< Args>( args)...} {}


            template< typename... Requested>
            void add( const Uuid& correlation, platform::ipc::id::type destination, Requested&&... requested)
            {
               m_messages.emplace_back(
                     destination,
                     correlation,
                     std::forward< Requested>( requested)...);


               if( m_messages.back().policy.done())
               {
                  m_policy( m_messages.back().queue, m_messages.back().message);
                  m_messages.pop_back();
               }
            }

            template< typename Reply>
            bool accumulate( Reply&& message)
            {
               auto found = range::find_if( m_messages, [&message]( const holder_type& m){
                  return m.message.correlation == message.correlation;
               });

               if( found && found->policy.consume( message.process.pid))
               {
                  //
                  // Accumulate message
                  //
                  m_policy( found->message, message);


                  if( found->policy.done())
                  {
                     //
                     // We're done, send reply...
                     //
                     m_policy( found->queue, found->message);
                     m_messages.erase( std::begin( found));
                  }

                  return true;
               }
               return false;
            }

            void remove( platform::pid::type pid)
            {
               range::trim( m_messages, range::remove_if( m_messages, [=]( holder_type& h){
                  if( h.policy.consume( pid) && h.policy.done())
                  {
                     m_policy( h.queue, h.message);
                     return true;
                  }
                  return false;
               }));
            }

            Policy& policy() { return m_policy;}
            const Policy& policy() const { return m_policy;}

            std::size_t size() const { return m_messages.size();}

         private:

            struct holder_type
            {
               template< typename... Args>
               holder_type( platform::ipc::id::type queue, const Uuid& correlation, Args&&... args)
                  : queue{ queue}, policy{ std::forward< Args>( args)...}
               {
                  message.correlation = correlation;
               }

               bool done() const { return policy.done();}

               platform::ipc::id::type queue;
               message_policy_type policy;
               Message message;
            };


            std::vector< holder_type> m_messages;
            Policy m_policy;
         };


      } // message
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_COORDINATE_H_
