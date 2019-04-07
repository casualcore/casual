//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/mockup/process.h"

namespace casual
{
   namespace eventually
   {
      namespace send
      {
         namespace unittest
         {
            //! @return a unittest process for `casual-eventually-send`
            struct Process : common::mockup::Process
            {
               Process();
            };
         } // unittest
      } // send
   } // eventually
} // casual