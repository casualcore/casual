//!
//! server.h
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
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

               struct Reply : basic_id< Type::server_ping_reply>
               {
                  Uuid uuid;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     basic_id< Type::server_ping_reply>::marshal( archive);
                     archive & uuid;
                  })

               };

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
