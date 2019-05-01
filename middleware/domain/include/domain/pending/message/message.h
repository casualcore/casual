//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"
#include "common/message/pending.h"

namespace casual
{
   namespace domain
   {
      namespace pending
      {
         namespace message
         {
            struct Connect : common::message::basic_message< common::message::Type::domain_pending_send_connect>
            {
               common::process::Handle process;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & process;
               })
               friend std::ostream& operator << ( std::ostream& out, const Connect& rhs);
            };

            struct Request : common::message::basic_message< common::message::Type::domain_pending_send_request>
            {
               Request( common::message::pending::Message&& message)
                  : message{ std::move( message)} {}
               
               Request() = default;

               common::message::pending::Message message;

               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & message;
               })
               friend std::ostream& operator << ( std::ostream& out, const Request& rhs);
            };
         } // message
      } // pending
   } // domain
} // casual