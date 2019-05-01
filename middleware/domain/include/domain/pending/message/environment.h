//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/uuid.h"

namespace casual
{
   namespace domain
   {
      namespace pending
      {
         namespace message
         {
            namespace environment
            {               
               //! identification of the instance
               const common::Uuid identification{ "e32ece3e19544ae69aae5a6326a3d1e9"};
               constexpr auto variable = "CASUAL_DOMAIN_PENDING_MESSAGE_PROCESS";

               constexpr auto executable = "casual-domain-pending-message";
            } // environment
         } // message
         
      } // pending
   } // domain
} // casual