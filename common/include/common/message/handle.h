//!
//! handle.h
//!
//! Created on: Sep 26, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MESSAGE_HANDLE_H_
#define CASUAL_COMMON_MESSAGE_HANDLE_H_


#include "common/message/server.h"
#include "common/queue.h"
#include "common/process.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace handle
         {
            //!
            //! Handles and discard a given message type
            //!
            template< typename M>
            struct Discard
            {
               Discard() = default;

               using message_type = M;

               void dispatch( message_type& message)
               {
                  // no op
               }
            };

            template< typename P>
            struct Ping
            {
               using queue_policy = P;

               using queue_type = common::queue::blocking::basic_writer< queue_policy>;

               using message_type = server::ping::Request;

               template< typename... Args>
               Ping( Args&&... args)
                  :  m_send( std::forward< Args>( args)...) {}

               void dispatch( message_type& message)
               {
                  server::ping::Reply reply;

                  reply.server = server::Id::current();
                  reply.uuid = process::uuid();

                  m_send( reply, message.server.queue_id);
               }

            private:
               queue_type m_send;
            };

            template< typename S>
            auto ping( S& state) -> Ping< common::queue::policy::RemoveOnTerminate< S>>
            {
               return Ping< common::queue::policy::RemoveOnTerminate< S>>{ state};
            }


         } // handle


      } // message
   } // common


} // casual

#endif // HANDLE_H_
