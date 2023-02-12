//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/model.h"
#include "configuration/message.h"

namespace casual
{
   namespace domain::configuration
   {
      //! fetches the configuration model from domain-manager
      //! and normalize the model regarding environment variables
      casual::configuration::Model fetch();

      namespace registration
      {
         using Ability = casual::configuration::message::stakeholder::registration::Ability;
         using Contract = casual::configuration::message::stakeholder::registration::Contract;

         void apply( Contract contract);
      } // registration

   } // domain::configuration

} // casual