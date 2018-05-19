//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/code/signal.h"
#include "common/exception/common.h"

namespace casual
{
   namespace common
   {
      namespace exception 
      {
         namespace signal 
         {

            using exception = common::exception::base_error< code::signal>;

            template< code::signal error>
            using base = common::exception::basic_error< exception, error>;
            
            using Timeout = base< code::signal::alarm>;
            using Terminate = base< code::signal::terminate>;
            using User = base< code::signal::user>;
            using Pipe = base< code::signal::pipe>;

            namespace child
            {
               using Terminate = base< code::signal::child>;
            } // child

         } // signal 
      } // exception 
   } // common
} // casual


