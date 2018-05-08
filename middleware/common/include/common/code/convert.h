//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/code/tx.h"
#include "common/code/xa.h"


namespace casual
{
   namespace common
   {
      namespace code
      {
         namespace convert
         {
            namespace to
            {
               code::tx tx( code::xa code);
            } // to
         } // convert
      } // code
   } // common
} // casual

