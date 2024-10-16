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
         using Code = common::service::Code;
         namespace code
         {
            constexpr auto initialize()
            {
               return Code{ common::code::xatmi::absent, 0L};
            }
         } // code

         namespace duplex
         {
            enum class Type : short
            {
               send,
               receive,
               terminated
            };
            inline std::ostream& operator << ( std::ostream& out, Type value)
            {
               switch( value)
               {
                  case Type::receive: return out << "receive";
                  case Type::send: return out << "send";
                  case Type::terminated: return out << "terminated";
               }
               return out << "<unknown>";
            }
         } // duplex


         namespace connect
         {
            namespace v1_2
            {
               namespace callee
               {
                  using base_type = message::basic_request< Type::conversation_connect_request_v2>;
                  struct Request : base_type
                  {
                     using base_type::base_type;

                     service::call::v1_2::Service service;
                     std::string parent;
                     common::transaction::ID trid;
                     common::service::header::Fields header;
                     platform::time::unit pending{};
                     duplex::Type duplex{};
                     common::buffer::Payload buffer;

                     CASUAL_CONST_CORRECT_SERIALIZE(
                        base_type::serialize( archive);
                        CASUAL_SERIALIZE( service);
                        CASUAL_SERIALIZE( parent);
                        CASUAL_SERIALIZE( trid);
                        CASUAL_SERIALIZE( header);
                        CASUAL_SERIALIZE( pending);
                        CASUAL_SERIALIZE( duplex);
                        CASUAL_SERIALIZE( buffer);
                     )
                  };

               } // callee
            } // v1_2


            using base_type = message::basic_request< Type::conversation_connect_request>;
            struct base_request : base_type
            {
               using base_type::base_type;

               service::call::Service service;
               execution::context::Parent parent;
               service::call::Deadline deadline;

               common::transaction::ID trid;
               common::service::header::Fields header;

               //! pending time, only to be return in the "ACK", to collect
               //! metrics
               platform::time::unit pending{};

               duplex::Type duplex{};

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_type::serialize( archive);
                  CASUAL_SERIALIZE( service);
                  CASUAL_SERIALIZE( parent);
                  CASUAL_SERIALIZE( deadline);
                  CASUAL_SERIALIZE( trid);
                  CASUAL_SERIALIZE( header);
                  CASUAL_SERIALIZE( pending);
                  CASUAL_SERIALIZE( duplex);
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


            using base_reply = basic_process< Type::conversation_connect_reply>;
            struct Reply : base_reply
            {
               using base_reply::base_reply;

               common::service::Code code = code::initialize();

               CASUAL_CONST_CORRECT_SERIALIZE(
                  base_reply::serialize( archive);
                  CASUAL_SERIALIZE( code);
               )
            };

         } // connect


         using send_base = basic_message< Type::conversation_send>;
         struct basic_send : send_base
         {
            duplex::Type duplex{};
            service::transaction::State transaction_state = service::transaction::State::ok;
            common::service::Code code = code::initialize();

            CASUAL_CONST_CORRECT_SERIALIZE(
               send_base::serialize( archive);
               CASUAL_SERIALIZE( duplex);
               CASUAL_SERIALIZE( transaction_state);
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

         using disconnect_base = basic_reply< Type::conversation_disconnect>;
         struct Disconnect : disconnect_base
         {
            CASUAL_CONST_CORRECT_SERIALIZE(
               disconnect_base::serialize( archive);
            )
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


