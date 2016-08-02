//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_BROKER_INCLUDE_BROKER_MESSAGE_H_
#define CASUAL_MIDDLEWARE_BROKER_INCLUDE_BROKER_MESSAGE_H_

#include "common/message/type.h"

namespace casual
{
   namespace broker
   {
      namespace message
      {

         namespace forward
         {
            namespace connect
            {
               using Request = common::message::server::connect::basic_request< common::message::Type::forward_connect_request>;
               using Reply = common::message::server::connect::basic_reply< common::message::Type::forward_connect_reply>;
            } // connect

         } // forward
      } // message

   } // broker

   namespace common
   {
      namespace message
      {
         template<>
         struct reverse::type_traits< broker::message::forward::connect::Request> : reverse::detail::type< broker::message::forward::connect::Reply> {};

      } // message
   } // common


} // casual

#endif // CASUAL_MIDDLEWARE_BROKER_INCLUDE_BROKER_MESSAGE_H_
