//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_COMMON_CODE_CONVERT_H_
#define CASUAL_COMMON_CODE_CONVERT_H_

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

#endif
