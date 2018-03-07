//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef COMMON_MESSAGE_SERVER_H_
#define COMMON_MESSAGE_SERVER_H_


#include "common/message/type.h"

#include "common/uuid.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace server
         {
            namespace ping
            {

               struct Request : basic_id< Type::server_ping_request>
               {

               };
               static_assert( traits::is_movable< Request>::value, "not movable");

               struct Reply : basic_id< Type::server_ping_reply>
               {
                  Uuid uuid;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_id< Type::server_ping_reply>::marshal( archive);
                     archive & uuid;
                  })
               };
               static_assert( traits::is_movable< Reply>::value, "not movable");

            } // ping

         } // server

         namespace reverse
         {
            template<>
            struct type_traits< server::ping::Request> : detail::type< server::ping::Reply> {};



         } // reverse


      } // message
   } //common
} // casual


#endif // SERVER_H_
