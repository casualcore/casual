//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "xatmi.h"

#include "common/algorithm.h"

#include <locale>

namespace casual
{

   namespace example
   {
      namespace resource
      {
         namespace server
         {

            extern "C"
            {
               void casual_example_resource_echo( TPSVCINFO* info)
               {
                  tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
               }
            } // C
         } // server
      } // resource
   } // example
} // casual