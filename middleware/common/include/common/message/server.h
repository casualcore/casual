//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/message/type.h"

#include "common/uuid.h"

namespace casual
{
   namespace common::message
   {
      namespace server::ping
      {
         using Request = basic_request< Type::server_ping_request>;
         using Reply = basic_process< Type::server_ping_reply>;
      } // server::ping

      namespace reverse
      {
         template<>
         struct type_traits< server::ping::Request> : detail::type< server::ping::Reply> {};
      } // reverse

   } //common::message
} // casual



