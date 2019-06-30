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
            using base_connect = common::message::basic_message< common::message::Type::domain_pending_send_request>;
            struct Connect : base_connect
            {
               common::process::Handle process;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  base_connect::serialize( archive);
                  CASUAL_SERIALIZE( process);
               })
            };
            using base_request = common::message::basic_message< common::message::Type::domain_pending_send_request>;
            struct Request : base_request
            {
               Request( common::message::pending::Message&& message)
                  : message{ std::move( message)} {}
               
               Request() = default;

               common::message::pending::Message message;

               CASUAL_CONST_CORRECT_SERIALIZE(
               {
                  base_request::serialize( archive);
                  CASUAL_SERIALIZE( message);
               })
            };

            namespace caller
            {
               //! wraps the pending message by reference
               struct Request : base_request
               {
                  Request( const common::message::pending::Message& message)
                     : message( message) {}
                  
                  const common::message::pending::Message& message;

                  CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
                  {
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( message);
                  })
               };
            } // caller

         } // message
      } // pending
   } // domain
} // casual