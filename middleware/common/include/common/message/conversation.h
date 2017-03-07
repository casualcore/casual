//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_CONVERSATION_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_CONVERSATION_H_

#include "common/message/service.h"

#include "common/service/conversation/flags.h"
#include "common/flag.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace conversation
         {
            struct Node
            {

               platform::ipc::id::type address = 0;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & address;
               })
               friend std::ostream& operator << ( std::ostream& out, const Node& value);
            };

            struct Route
            {
               std::vector< Node> nodes;

               inline Node next()
               {
                  auto node = std::move( nodes.back());
                  nodes.pop_back();
                  return node;
               }

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & nodes;
               })

               friend std::ostream& operator << ( std::ostream& out, const Route& value);
            };

            namespace connect
            {
               template< template< message::Type> class base>
               struct basic_request : base< Type::service_conversation_connect_request>
               {
                  using base_request = base< Type::service_conversation_connect_request>;
                  using base_request::base_request;

                  Route recording;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_request::marshal( archive);
                     archive & recording;
                  })
               };

               namespace caller
               {
                  using Request = basic_request< service::call::caller::basic_request>;

               } // caller

               namespace callee
               {
                  using Request = basic_request< service::call::callee::basic_request>;

               } // callee


               using reply_base = basic_reply< Type::service_conversation_connect_reply>;
               struct Reply : reply_base
               {
                  Route route;
                  Route recording;
                  int status;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     reply_base::marshal( archive);
                     archive & route;
                     archive & recording;
                     archive & status;
                  })
               };

            } // connect

            namespace send
            {
               using request_base = basic_request< Type::service_conversation_send_request>;
               struct base_request : request_base
               {
                  Route route;
                  common::service::conversation::send::Flags flags;
                  common::service::conversation::Events events;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     request_base::marshal( archive);
                     archive & route;
                     archive & flags;
                     archive & events;
                  })
                  friend std::ostream& operator << ( std::ostream& out, const base_request& value);
               };

               namespace caller
               {
                  using Request = message::buffer::caller::basic_request< base_request>;
               } // caller


               namespace callee
               {
                  using Request = message::buffer::callee::basic_request< base_request>;
               } // callee

               using reply_base = basic_reply< Type::service_conversation_send_reply>;
               struct Reply : reply_base
               {
                  Route route;

                  common::service::conversation::Events events;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     reply_base::marshal( archive);
                     archive & route;
                     archive & events;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Reply& value);
               };

            } // send


         } // conversation

         namespace reverse
         {

            template<>
            struct type_traits< conversation::connect::caller::Request> : detail::type< conversation::connect::Reply> {};

            template<>
            struct type_traits< conversation::connect::callee::Request> : detail::type< conversation::connect::Reply> {};

            template<>
            struct type_traits< conversation::send::callee::Request> : detail::type< conversation::send::Reply> {};

            template<>
            struct type_traits< conversation::send::caller::Request> : detail::type< conversation::send::Reply> {};

         } // reverse

      } // message
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_CONVERSATION_H_
