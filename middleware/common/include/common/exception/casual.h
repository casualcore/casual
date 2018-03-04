//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_COMMON_EXCEPTION_CASUAL_H_
#define CASUAL_COMMON_EXCEPTION_CASUAL_H_

#include "common/code/casual.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace casual 
         {

            using exception = common::exception::base_error< code::casual>;

            template< code::casual error>
            using base = common::exception::basic_error< exception, error>;
            
            using Shutdown = base< code::casual::shutdown>;

            namespace invalid
            {
               using Configuration = base< code::casual::invalid_configuration>;
               using Version = base< code::casual::invalid_version>;
            } // invalid

         } // casual 
      } // exception 
   } // common
} // casual



#endif
