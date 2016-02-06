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
         namespace outbound
         {
            struct Connect : common::message::basic_message< common::message::Type::gateway_outbound_connect>
            {
               std::string connection;

               CASUAL_CONST_CORRECT_MARSHAL({
                  archive & connection;
               })

            };

         } // outbound

      } // message

   } // gateway


} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MESSAGE_H_
