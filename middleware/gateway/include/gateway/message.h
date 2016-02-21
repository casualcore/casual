//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_

#include "common/message/type.h"

namespace casual
{
   namespace gateway
   {
      namespace message
      {
         struct Domain
         {
            common::Uuid id;
            std::string name;

            CASUAL_CONST_CORRECT_MARSHAL({
               archive & id;
               archive & name;
            })
         };

         namespace outbound
         {
            struct Connect : common::message::basic_message< common::message::Type::gateway_outbound_connect>
            {

               std::string connection;

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & connection;
               })

            };

         } // outbound

         namespace inbound
         {
            namespace ipc
            {
               namespace connect
               {
                  template< common::message::Type type>
                  struct basic_connect : common::message::basic_message< type>
                  {
                     common::process::Handle process;
                     message::Domain domain;

                     CASUAL_CONST_CORRECT_MARSHAL({
                        common::message::basic_message< type>::marshal( archive);
                        archive & process;
                        archive & domain;
                     })

                  };

                  struct Request : basic_connect< common::message::Type::gateway_inbound_ipc_connect_request>
                  {
                  };

                  struct Reply : basic_connect< common::message::Type::gateway_inbound_ipc_connect_reply>
                  {
                  };

               } // connect



            } // ipc

         } // inbound

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
            struct type_traits< gateway::message::inbound::ipc::connect::Request> : detail::type<  gateway::message::inbound::ipc::connect::Reply> {};


         } // reverse
      } // message
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_
