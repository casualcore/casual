//!
//! handle.h
//!
//! Created on: Sep 26, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_COMMON_MESSAGE_HANDLE_H_
#define CASUAL_COMMON_MESSAGE_HANDLE_H_


#include "common/message/server.h"
#include "common/process.h"
#include "common/internal/log.h"

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

               void operator () ( message_type& message)
               {
                  log::internal::debug << "discard message: " << message.type() << '\n';
               }
            };

            struct Ping
            {
               void operator () ( server::ping::Request& message);
            };


            inline auto ping() -> Ping
            {
               return Ping{};
            }


            struct Shutdown
            {
               using message_type = message::shutdown::Request;

               void operator () ( message_type& message);
            };

         } // handle
      } // message
   } // common


} // casual

#endif // HANDLE_H_
