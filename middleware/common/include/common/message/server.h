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
   namespace common
   {
      namespace message
      {
         namespace server
         {
            namespace ping
            {

               using Request = basic_request< Type::server_ping_request>;

               using base_reply = basic_reply< Type::server_ping_reply>;
               struct Reply : base_reply
               {
                  using base_reply::base_reply;
                  Uuid uuid;

                  CASUAL_CONST_CORRECT_SERIALIZE(
                  {
                     base_reply::serialize( archive);
                     CASUAL_SERIALIZE( uuid);
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



