//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_CONVERSATION_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_CONVERSATION_H_

#include "common/message/service.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace conversation
         {
            struct Node
            {

               platform::ipc::id::type address = 0;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & address;
               })
            };

            struct Route
            {
               std::vector< Node> nodes;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & nodes;
               })
            };

            namespace connect
            {
               using rquest_base = service::call::basic_call< Type::service_conversation_connect_request>;
               struct Request : rquest_base
               {
                  Route route;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     rquest_base::marshal( archive);
                     archive & route;
                  })
               };

               using reply_base = basic_reply< Type::service_conversation_connect_request>;
               struct Reply : reply_base
               {
                  Route route;

                  CASUAL_CONST_CORRECT_MARSHAL(
                  {
                     reply_base::marshal( archive);
                     archive & route;
                  })
               };

            } // connect

            namespace send
            {


            } // send

         } // conversation



      } // message
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MESSAGE_CONVERSATION_H_
