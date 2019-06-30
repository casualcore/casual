//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/service.h"

#include "common/flag/service/conversation.h"


#include <cassert>

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
               inline Node( strong::ipc::id address) : address( address) {}

               strong::ipc::id address;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( address);
               })
            };

            struct Route
            {
               std::vector< Node> nodes;

               inline Node next()
               {
                  assert( ! nodes.empty());

                  auto node = std::move( nodes.back());
                  nodes.pop_back();
                  return node;
               }

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  CASUAL_SERIALIZE( nodes);
               })
            };

            namespace connect
            {

               using base_request = type_wrapper< service::call::common_request, Type::service_conversation_connect_request>;
               struct basic_request : base_request
               {
                  Route recording;
                  flag::service::conversation::connect::Flags flags;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( recording);
                     CASUAL_SERIALIZE( flags);
                  })
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
                  service::Code code;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     reply_base::serialize( archive);
                     CASUAL_SERIALIZE( route);
                     CASUAL_SERIALIZE( recording);
                     CASUAL_SERIALIZE( code);
                  })
               };

            } // connect

            using send_base = basic_request< Type::service_conversation_send>;
            struct basic_send : send_base
            {
               Route route;
               
               flag::service::conversation::send::Flags flags;
               flag::service::conversation::Events events;
               service::Transaction transaction;
               service::Code code;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  send_base::serialize( archive);
                  CASUAL_SERIALIZE( route);
                  CASUAL_SERIALIZE( flags);
                  CASUAL_SERIALIZE( events);
                  CASUAL_SERIALIZE( transaction);
                  CASUAL_SERIALIZE( code);
               })
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
               flag::service::conversation::Events events;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  disconnect_base::serialize( archive);
                  CASUAL_SERIALIZE( route);
                  CASUAL_SERIALIZE( events);
               })
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


