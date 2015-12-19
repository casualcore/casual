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
                  log::internal::debug << "discard message: " << message.message_type << '\n';
               }
            };

            template< typename P>
            struct Ping
            {
               using queue_policy = P;

               using queue_type = common::queue::blocking::basic_send< queue_policy>;

               template< typename... Args>
               Ping( Args&&... args)
                  :  m_send( std::forward< Args>( args)...) {}

               void operator () ( server::ping::Request& message)
               {
                  log::internal::debug << "pinged by process: " << message.process << '\n';

                  server::ping::Reply reply;
                  reply.correlation = message.correlation;
                  reply.process = process::handle();
                  reply.uuid = process::uuid();

                  m_send( message.process.queue, reply);
               }

            private:
               queue_type m_send;
            };

            inline auto ping( common::queue::policy::callback::on::Terminate::callback_type callback)
               -> Ping< common::queue::policy::callback::on::Terminate>
            {
               return Ping< common::queue::policy::callback::on::Terminate>{ std::move( callback)};
            }

            inline auto ping() -> Ping< common::queue::policy::NoAction>
            {
               return Ping< common::queue::policy::NoAction>{};
            }


            struct Shutdown
            {
               using message_type = message::shutdown::Request;

               void operator () ( message_type& message);
            };



            namespace connect
            {
               template< typename Q, typename C, typename M>
               auto reply( Q&& queue, C&& correlation, M&& message) -> decltype( std::forward< M>( message))
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
            } // connect

         } // handle


      } // message
   } // common


} // casual

#endif // HANDLE_H_
