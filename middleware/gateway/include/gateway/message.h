//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_

#include "common/message/type.h"
#include "common/domain.h"

namespace casual
{
   namespace gateway
   {
      namespace message
      {

         namespace outbound
         {
            struct Connect : common::message::basic_message< common::message::Type::gateway_outbound_connect>
            {
               common::process::Handle process;
               common::domain::Identity remote;

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & process;
                  archive & remote;

               })

            };

         } // outbound

         namespace ipc
         {
            namespace connect
            {
               template< common::message::Type type>
               struct basic_connect : common::message::basic_message< type>
               {
                  common::process::Handle process;
                  common::domain::Identity remote;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     common::message::basic_message< type>::marshal( archive);
                     archive & process;
                     archive & remote;
                  })

               };

               struct Request : basic_connect< common::message::Type::gateway_ipc_connect_request>
               {
               };

               struct Reply : basic_connect< common::message::Type::gateway_ipc_connect_reply>
               {
               };

            } // connect



         } // ipc


         namespace worker
         {

            struct Connect : common::message::basic_message< common::message::Type::gateway_worker_connect>
            {
               common::platform::binary_type information;

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & information;
               })

            };

            struct Disconnect : common::message::basic_message< common::message::Type::gateway_worker_disconnect>
            {
            };


         } // worker


      } // message

   } // gateway

   namespace common
   {
      namespace message
      {
         namespace reverse
         {
            template<>
            struct type_traits< gateway::message::ipc::connect::Request> : detail::type<  gateway::message::ipc::connect::Reply> {};


         } // reverse
      } // message
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_
