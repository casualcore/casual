//!
//! casual
//!

#ifndef CASUAL_COMMON_EXCEPTION_XA_H_
#define CASUAL_COMMON_EXCEPTION_XA_H_

#include "common/code/xa.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace ax 
         {

            using exception = common::exception::base_error< code::ax>;

            template< code::ax error>
            using base = common::exception::basic_error< exception, error>;

            int handle();
         } // xa 

         namespace xa 
         {

            using exception = common::exception::base_error< code::xa>;

            template< code::xa error>
            using base = common::exception::basic_error< exception, error>;

            int handle();
         } // xa 
      } // exception 
   } // common
} // casual



#endif
