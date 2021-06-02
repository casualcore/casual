//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/model.h"

namespace casual
{
   namespace domain::configuration
   {
      //! fetches the configuration model from domain-manager
      //! and normalize the model regarding environment variables
      casual::configuration::Model fetch();

      namespace supplier
      {
         //! registrate that this _process_ can supply configuration
         void registration();
      } // supplier

   } // domain::configuration

} // casual