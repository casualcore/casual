//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "eventually/send/unittest/process.h"

#include "common/environment.h"

namespace casual
{
   namespace eventually
   {
      namespace send
      {
         namespace unittest
         {
            Process::Process() : common::mockup::Process{ 
               common::environment::variable::get( "CASUAL_REPOSITORY_ROOT") + "/middleware/eventually/bin/casual-eventually-send"}
            {
            };

         } // unittest
      } // send
   } // eventually
} // casual