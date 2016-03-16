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

            friend std::ostream& operator << ( std::ostream& out, const basic_connect& value)
            {
               return out << "{ process: " << value.process
                     << ", remote: " << value.remote
                     << '}';
            }
         };

         namespace outbound
         {
            struct Connect : basic_connect< common::message::Type::gateway_outbound_connect>
            {
            };

         } // outbound

         namespace inbound
         {
            struct Connect : basic_connect< common::message::Type::gateway_inbound_connect>
            {
            };
         } // inbound

         namespace ipc
         {
            namespace connect
            {

               struct Request : basic_connect< common::message::Type::gateway_ipc_connect_request>
               {
                  friend std::ostream& operator << ( std::ostream& out, const Request& value);
               };

               struct Reply : basic_connect< common::message::Type::gateway_ipc_connect_reply>
               {
                  friend std::ostream& operator << ( std::ostream& out, const Reply& value);
               };

            } // connect



         } // ipc


         namespace worker
         {

            struct Connect : common::message::basic_message< common::message::Type::gateway_worker_connect>
            {
               common::platform::binary_type information;
               common::domain::Identity remote;

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & information;
                  archive & remote;
               })

            };

            struct Disconnect : common::message::basic_message< common::message::Type::gateway_worker_disconnect>
            {
               enum class Reason : char
               {
                  invalid,
                  signal,
                  disconnect
               };

               Disconnect() = default;
               Disconnect( Reason reason) : reason( reason) {}

               common::domain::Identity remote;
               Reason reason = Reason::invalid;

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & remote;
                  archive & reason;
               })

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
