//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/manager/state.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace state
         {
            namespace create
            {
               namespace boot 
               {
                  std::vector< state::Batch> order( 
                     const std::vector< Server>& servers, 
                     const std::vector< Executable>& executables,
                     const std::vector< Group>& groups);
               } // dependency   
               
            } // create
         } // state
      } // manager
   } // domain
} // casual