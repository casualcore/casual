//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "casual/argument.h"
#include "common/pimpl.h"

namespace casual
{
   namespace configuration::admin
   {
      //! exposed for domain cli, to keep compatibility... 
      //! @remove in 2.0
      namespace deprecated
      {
         argument::Option get();
         argument::Option post();
         argument::Option put();
         argument::Option edit();

      } // deprecated

      namespace cli
      {
         argument::Option options();
      } // cli
      
   } // configuration::admin
} // casual
