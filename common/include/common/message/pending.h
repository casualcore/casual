//!
//! pending.h
//!
//! Created on: Oct 12, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MESSAGE_PENDING_H_
#define CASUAL_COMMON_MESSAGE_PENDING_H_

#include "common/ipc.h"
#include "common/marshal/binary.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace pending
         {

            //!
            //! A pending message, that will be sent later.
            //!
            struct Message
            {
               using targets_type = std::vector< platform::queue_id_type>;

               enum class Targets
               {
                  all,
                  first
               };

               template< typename M>
               Message( M&& message, targets_type targets, Targets task)
                  : targets{ std::move( targets)}, complete{ marshal::complete( std::forward< M>( message))}, task{ task}
               {
               }

               template< typename M>
               Message( M&& message, targets_type targets)
                  : Message( std::move( message), std::move( targets), Targets::all)
               {
               }

               Message( Message&&) = default;
               Message& operator = ( Message&&) = default;

               bool sent() const
               {
                  return targets.empty();
               }

               targets_type targets;
               common::ipc::message::Complete complete;
               Targets task;
            };


            //!
            //! Tries to send a message to targets.
            //! Depending on the task it will either send to all
            //! or stop when the first is successful.
            //!
            template< typename Q>
            struct Send
            {
               Send( Q& queue) : m_queue( queue) {}

               bool operator () ( Message& message)
               {

                  auto send = [&]( platform::queue_id_type ipc){
                     return m_queue.send( ipc, message.complete);
                  };

                  if( message.task == Message::Targets::all)
                  {
                     message.targets = range::to_vector(
                           range::remove_if( message.targets, send));
                  }
                  else
                  {
                     if( range::find_if( message.targets, send))
                     {
                        message.targets.clear();
                     }
                  }

                  return message.sent();
               }
            private:
               Q& m_queue;
            };

            //!
            //! @return a 'pending-sender' that tries to send to targets
            //!
            template< typename Q>
            Send< Q> sender( Q& queue)
            {
               return Send< Q>( queue);
            }
         } // pending

      } // message
   } // common


} // casual

#endif // PENDING_H_
