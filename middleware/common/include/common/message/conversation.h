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
   namespace common::message
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
            struct base_request : service::call::common_request< Type::service_conversation_connect_request>
            {
               using base = service::call::common_request< Type::service_conversation_connect_request>;
               
               Route recording;
               flag::service::conversation::connect::Flags flags;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base::serialize( archive);
                  CASUAL_SERIALIZE( recording);
                  CASUAL_SERIALIZE( flags);
               )
            };

            namespace caller
            {
               struct Request : base_request
               {
                  template< typename... Args>
                  Request( common::buffer::payload::Send buffer, Args&&... args)
                     : base_request( std::forward< Args>( args)...), buffer( std::move( buffer))
                  {}
                  common::buffer::payload::Send buffer;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( buffer);
                  )
               };

            } // caller

            namespace callee
            {
               struct Request : base_request
               {
                  using base_request::base_request;

                  common::buffer::Payload buffer;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( buffer);
                  )
               };

            } // callee


            using base_reply = basic_reply< Type::service_conversation_connect_reply>;
            struct Reply : base_reply
            {
               Route route;
               Route recording;
               service::Code code;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( route);
                  CASUAL_SERIALIZE( recording);
                  CASUAL_SERIALIZE( code);
               )
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
               send_base::serialize( archive);
               CASUAL_SERIALIZE( route);
               CASUAL_SERIALIZE( flags);
               CASUAL_SERIALIZE( events);
               CASUAL_SERIALIZE( transaction);
               CASUAL_SERIALIZE( code);
            )
         };

         namespace caller
         {
            struct Send : basic_send
            {
               template< typename... Args>
               Send( common::buffer::payload::Send buffer, Args&&... args)
                  : basic_send( std::forward< Args>( args)...), buffer( std::move( buffer))
               {}
               common::buffer::payload::Send buffer;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  basic_send::serialize( archive);
                  CASUAL_SERIALIZE( buffer);
               )
            };

         } // caller


         namespace callee
         {
            struct Send : basic_send
            {
               using basic_send::basic_send;

               common::buffer::Payload buffer;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  basic_send::serialize( archive);
                  CASUAL_SERIALIZE( buffer);
               )
            };
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

   } // common::message
} // casual


