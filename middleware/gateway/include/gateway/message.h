//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_

#include "common/message/type.h"
#include "common/domain.h"

#include <thread>

namespace casual
{
   namespace gateway
   {
      namespace message
      {

         namespace manager
         {

            namespace listener
            {
               struct Event : common::message::basic_message< common::message::Type::gateway_manager_listener_event>
               {

                  enum class State
                  {
                     running,
                     exit,
                     signal,
                     error
                  };

                  State state;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     base_type::marshal( archive);
                     archive & state;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const Event::State& value);
                  friend std::ostream& operator << ( std::ostream& out, const Event& value);
               };

            } // listener
         } // manager


         template< common::message::Type type>
         struct basic_connect : common::message::basic_message< type>
         {
            common::process::Handle process;

            CASUAL_CONST_CORRECT_MARSHAL({
               common::message::basic_message< type>::marshal( archive);
               archive & process;
            })

            friend std::ostream& operator << ( std::ostream& out, const basic_connect& value)
            {
               return out << "{ process: " << value.process
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

         namespace tcp
         {
            struct Connect : common::message::basic_message< common::message::Type::gateway_manager_tcp_connect>
            {
               common::platform::tcp::descriptor::type descriptor;

               friend std::ostream& operator << ( std::ostream& out, const Connect& value);

               CASUAL_CONST_CORRECT_MARSHAL({
                  base_type::marshal( archive);
                  archive & descriptor;
               })
            };

         } // tcp


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

               friend std::ostream& operator << ( std::ostream& out, Reason value);
               friend std::ostream& operator << ( std::ostream& out, const Disconnect& value);

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
