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



            namespace connect
            {
               /*
               template< typename D, typename C, typename M>
               auto reply( D&& device, C&& correlation, M&& message) -> decltype( std::forward< M>( message))
               {
                  queue( message, correlation);

                  switch( message.directive)
                  {
                     case M::Directive::singleton:
                     {
                        log::error << "broker denied startup - reason: executable is a singleton - action: terminate\n";
                        throw exception::Shutdown{ "broker denied startup - reason: process is regarded a singleton - action: terminate"};
                     }
                     case M::Directive::shutdown:
                     {
                        log::error << "broker denied startup - reason: broker is in shutdown mode - action: terminate\n";
                        throw exception::Shutdown{ "broker denied startup - reason: broker is in shutdown mode - action: terminate"};
                     }
                     default:
                     {
                        break;
                     }
                  }
                  return std::forward< M>( message);
               }
               */

               template< typename M>
               auto reply( M&& message) -> decltype( std::forward< M>( message))
               {
                  using message_type = typename std::decay< M>::type;

                  switch( message.directive)
                  {
                     case message_type::Directive::singleton:
                     {
                        log::error << "broker denied startup - reason: executable is a singleton - action: terminate\n";
                        throw exception::Shutdown{ "broker denied startup - reason: process is regarded a singleton - action: terminate"};
                     }
                     case message_type::Directive::shutdown:
                     {
                        log::error << "broker denied startup - reason: broker is in shutdown mode - action: terminate\n";
                        throw exception::Shutdown{ "broker denied startup - reason: broker is in shutdown mode - action: terminate"};
                     }
                     default:
                     {
                        break;
                     }
                  }
                  return std::forward< M>( message);
               }
            } // connect

         } // handle


      } // message
   } // common


} // casual

#endif // HANDLE_H_
