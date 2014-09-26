//!
//! handle.h
//!
//! Created on: Sep 26, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MESSAGE_HANDLE_H_
#define CASUAL_COMMON_MESSAGE_HANDLE_H_


#include "common/message/server.h"

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

            /*
            template< typename Q>
            struct Ping
            {
               using queue_writer = Q;

               using message_type = server::ping::Request::message_type;

               Ping( queue_writer& queue)

               void dispatch( message_type& message)
               {
                  server::ping::Reply reply;


                  queue_writer writer
                  reply.server = message.server;

                  m_writer( reply);
               }
            };

            template< typename Q>
            Ping< Q> ping( Q& queue)
            {
               return Ping< Q>{};
            }
            */


         } // handle
      } // message
   } // common


} // casual

#endif // HANDLE_H_
