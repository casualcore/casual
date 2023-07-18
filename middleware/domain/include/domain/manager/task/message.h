//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"
#include "common/domain.h"

namespace casual
{
   namespace domain::manager::task::message
   {
      namespace domain
      {
         using base_information= common::message::basic_request< common::message::Type::event_domain_information>;
         struct Information : base_information
         {
            using base_information::base_information;

            common::domain::Identity domain;

            CASUAL_CONST_CORRECT_SERIALIZE({
               base_information::serialize( archive);
               CASUAL_SERIALIZE( domain);
            })
         };
      } // domain

   } // domain::manager::task::message

} // casual