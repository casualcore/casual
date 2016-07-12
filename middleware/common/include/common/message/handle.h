//!
//! casaul
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

            //!
            //! Replies to a ping message
            //!
            struct Ping
            {
               void operator () ( server::ping::Request& message);
            };


            inline auto ping() -> Ping
            {
               return Ping{};
            }



            //!
            //! @throws exception::Shutdown if message::shutdown::Request is dispatched
            //!
            struct Shutdown
            {
               using message_type = message::shutdown::Request;

               void operator () ( message_type& message);
            };

            //!
            //! Dispatch and assigns a given message
            //!
            template< typename M>
            struct Assign
            {
               using message_type = M;

               Assign( message_type& message) : m_message( message) {}

               void operator () ( message_type& message)
               {
                  m_message = message;
               }
            private:
               message_type& m_message;
            };

            template< typename M>
            Assign< M> assign( M& message)
            {
               return Assign< M>{ message};
            }


         } // handle
      } // message
   } // common


} // casual

#endif // HANDLE_H_
