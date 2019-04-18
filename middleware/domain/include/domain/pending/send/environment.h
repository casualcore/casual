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
         namespace send
         {
            //! identification of the instance
            const common::Uuid identification{ "e32ece3e19544ae69aae5a6326a3d1e9"};
            constexpr auto environment = "CASUAL_EVENTUALLY_SEND_PROCESS";

            constexpr auto executable = "casual-domain-pending-send";

         } // send
         
      } // pending
   } // domain
} // casual