//!
//! pending.h
//!
//! Created on: Oct 12, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MESSAGE_PENDING_H_
#define CASUAL_COMMON_MESSAGE_PENDING_H_

#include "common/ipc.h"

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

               template< typename M>
               Message( M&& message, targets_type targets) : targets( std::move( targets))
               {
                  common::marshal::output::Binary archive;
                  archive << message;

                  auto type = common::message::type( message);
                  complete = common::ipc::message::Complete( type, archive.release());
               }

               Message( Message&&) = default;
               Message& operator = ( Message&&) = default;

               bool sent() const
               {
                  return targets.empty();
               }

               targets_type targets;
               common::ipc::message::Complete complete;
            };


            template< typename Q>
            struct Send
            {
               Send( Q& queue) : m_queue( queue) {}

               bool operator () ( Message& message)
               {

                  auto send = [&]( platform::queue_id_type ipc){
                     return m_queue.send( ipc, message.complete);
                  };

                  message.targets = range::to_vector(
                        range::remove_if( message.targets, send));

                  return message.sent();
               }
            private:
               Q& m_queue;
            };

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
