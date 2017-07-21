//!
//! casual
//!

#ifndef CASUAL_COMMON_EXCEPTION_XA_H_
#define CASUAL_COMMON_EXCEPTION_XA_H_

#include "common/error/code/xa.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace xa 
         {

            using exception = common::exception::base_error< error::code::xa>;

            template< error::code::xa error>
            using base = common::exception::basic_error< exception, error>;
            
         } // xa 
      } // exception 
   } // common
} // casual



#endif