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

               /*
               namespace create
               {

                  struct Request : common::message::basic_message< common::message::Type::gateway_manager_listener_create_reqeust>
                  {
                     common::communication::tcp::Address address;

                     CASUAL_CONST_CORRECT_MARSHAL({
                        base_type::marshal( archive);
                        archive & address;
                     })

                     friend std::ostream& operator << ( std::ostream& out, const Request& value);

                  };

               } // create
               */


            } // listener


         } // manager


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


         namespace connection
         {
            namespace information
            {
               template< common::message::Type type>
               struct basic_information : common::message::basic_message< type>
               {
                  common::process::Handle process;
                  common::domain::Identity remote;
                  std::vector< std::string> addresses;

                  CASUAL_CONST_CORRECT_MARSHAL({
                     common::message::basic_message< type>::marshal( archive);
                     archive & process;
                     archive & remote;
                     archive & addresses;
                  })

                  friend std::ostream& operator << ( std::ostream& out, const basic_information& value)
                  {
                     return out << "{ process: " << value.process
                           << ", remote: " << value.remote
                           << ", addresses: " << common::range::make( value.addresses)
                           << '}';
                  }
               };

               //!
               //! Used to exchange domain information between domains
               //!
               //! outbound is always the one who sends the request
               //!
               //! @{
               using Request =  basic_information< common::message::Type::gateway_connection_information_request>;
               using Reply =  basic_information< common::message::Type::gateway_connection_information_reply>;
               //! @}

            } // information

         } // connection





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

            template<>
            struct type_traits< gateway::message::connection::information::Request> : detail::type< gateway::message::connection::information::Reply> {};


         } // reverse
      } // message
   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_
