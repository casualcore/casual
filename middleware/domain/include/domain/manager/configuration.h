//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/state.h"
#include "domain/manager/admin/model.h"

#include "configuration/model.h"

#include "casual/manager/service/protocol.h"

namespace casual
{
   namespace domain::manager::configuration
   {
      
      //! @returns the total state of all managers.
      casual::configuration::Model get( State& state);

      //! @pre state.configuration.model is set to the current aggregated configuration model.
      void post( casual::manager::service::protocol::concurrent::Finalize< void> finalize, State& state, casual::configuration::Model wanted);


   } // domain::manager::configuration
} // casual


