//!
//! casual
//!

#ifndef CASUAL_COMMON_EXCEPTION_SIGNAL_H_
#define CASUAL_COMMON_EXCEPTION_SIGNAL_H_

#include "common/error/code/signal.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace signal 
         {

            using exception = common::exception::base_error< error::code::signal>;

            template< error::code::signal error>
            using base = common::exception::basic_error< exception, error>;
            
            using Timeout = base< error::code::signal::alarm>;
            using Terminate = base< error::code::signal::terminate>;
            using User = base< error::code::signal::user>;
            using Pipe = base< error::code::signal::pipe>;

            namespace child
            {
               using Terminate = base< error::code::signal::child>;
            } // child

         } // signal 
      } // exception 
   } // common
} // casual



#endif