//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "transaction/manager/state.h"

#include "configuration/model.h"
#include "configuration/message.h"


namespace casual
{
   namespace transaction::manager::configuration
   {
      void conform( State& state, casual::configuration::Model wanted);
      
   } // transaction::manager::configuration
   
} // casual
