//!
//! casual
//!

#ifndef CASUAL_COMMON_ERROR_CODE_CONVERT_H_
#define CASUAL_COMMON_ERROR_CODE_CONVERT_H_

#include "common/error/code/tx.h"
#include "common/error/code/xa.h"


namespace casual
{
   namespace common
   {
      namespace error
      {
         namespace code 
         {
            namespace convert
            {
               namespace to
               {
                  error::code::tx tx( error::code::xa code);
               } // to
            } // convert            
         } // code 
      } // error
   } // common
} // casual

#endif