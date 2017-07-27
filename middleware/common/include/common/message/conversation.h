//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_CONVERSATION_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_CONVERSATION_H_

#include "common/message/service.h"

#include "common/service/conversation/flags.h"
#include "common/flag.h"

#include "common/communication/ipc/handle.h"

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
               inline Node() = default;
               inline Node( communication::ipc::Handle address) : address( address) {}

               communication::ipc::Handle address;

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
               enum class Flag : long
               {
                  no_transaction = TPNOTRAN,
                  send_only = TPSENDONLY,
                  receive_only = TPRECVONLY,
                  no_time = TPNOTIME,
               };
               using Flags = common::Flags< Flag>;

               using base_request = type_wrapper< service::call::common_request, Type::service_conversation_connect_request>;
               struct basic_request : base_request
               {
                  Route recording;
                  Flags flags;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     base_request::marshal( archive);
                     archive & recording;
                     archive & flags;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const basic_request& value);
               };

               namespace caller
               {
                  using Request = message::buffer::caller::basic_request< basic_request>;

               } // caller

               namespace callee
               {
                  using Request = message::buffer::callee::basic_request< basic_request>;

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
                  friend std::ostream& operator << ( std::ostream& out, const Reply& value);
               };

            } // connect

            using send_base = basic_request< Type::service_conversation_send>;
            struct basic_send : send_base
            {
               Route route;
               common::service::conversation::send::Flags flags;
               common::service::conversation::Events events;
               service::Transaction transaction;
               int status;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  send_base::marshal( archive);
                  archive & route;
                  archive & flags;
                  archive & events;
                  archive & status;
               })
               friend std::ostream& operator << ( std::ostream& out, const basic_send& value);
            };

            namespace caller
            {
               using Send = message::buffer::caller::basic_request< basic_send>;
            } // caller


            namespace callee
            {
               using Send = message::buffer::callee::basic_request< basic_send>;
            } // callee


            using disconnect_base = basic_reply< Type::service_conversation_disconnect>;
            struct Disconnect : disconnect_base
            {
               Route route;
               common::service::conversation::Events events;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  disconnect_base::marshal( archive);
                  archive & route;
                  archive & events;
               })

               friend std::ostream& operator << ( std::ostream& out, const Disconnect& value);
            };


         } // conversation

         namespace reverse
         {

            template<>
            struct type_traits< conversation::connect::caller::Request> : detail::type< conversation::connect::Reply> {};

            template<>
            struct type_traits< conversation::connect::callee::Request> : detail::type< conversation::connect::Reply> {};


         } // reverse

      } // message
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_CONVERSATION_H_
