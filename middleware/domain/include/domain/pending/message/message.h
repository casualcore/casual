//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/pending.h"
#include "common/message/domain.h"

namespace casual
{
   namespace domain
   {
      namespace pending
      {
         namespace message
         {
            namespace connect
            {
               using Request = common::message::domain::process::connect::basic_request< common::message::Type::domain_pending_message_connect_request>;
               using Reply = common::message::domain::process::connect::basic_reply< common::message::Type::domain_pending_message_connect_reply>;

            } // connect


            using base_request = common::message::basic_message< common::message::Type::domain_pending_message_send_request>;
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

                  CASUAL_LOG_SERIALIZE(
                  {
                     base_request::serialize( archive);
                     CASUAL_SERIALIZE( message);
                  })
               };
            } // caller

         } // message
      } // pending
   } // domain

   namespace common
   {
      namespace message
      {
         namespace reverse
         {
            template<>
            struct type_traits< casual::domain::pending::message::connect::Request> : detail::type< casual::domain::pending::message::connect::Reply> {};

         } // reverse

      } // message
   } // common
} // casual