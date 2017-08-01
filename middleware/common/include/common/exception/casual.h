//!
//! casual
//!

#ifndef CASUAL_COMMON_EXCEPTION_CASUAL_H_
#define CASUAL_COMMON_EXCEPTION_CASUAL_H_

#include "common/error/code/casual.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace casual 
         {

            using exception = common::exception::base_error< error::code::casual>;

            template< error::code::casual error>
            using base = common::exception::basic_error< exception, error>;
            
            using Shutdown = base< error::code::casual::shutdown>;

            namespace invalid
            {
               using Configuration = base< error::code::casual::invalid_configuration>;
            } // invalid

         } // casual 
      } // exception 
   } // common
} // casual



#endif