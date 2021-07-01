//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "plugin/call.h"

namespace casual
{
   namespace plugin::call
   {  

      Context::Context( Arguments arguments)
      {

      }

      Context::~Context()
      {
         
      }


      std::optional< Reply> Context::receive()
      {
         return {};
      }


   } // plugin::call
} // casual