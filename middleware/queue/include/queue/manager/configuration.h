//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "queue/manager/state.h"

#include "configuration/message.h"


namespace casual
{
   namespace queue::manager::configuration
   {
      void conform( State& state, casual::configuration::model::queue::Model current, casual::configuration::model::queue::Model wanted);
      void conform( State& state, casual::configuration::message::update::Request&& message);
      
   } // queue::manager::configuration
   
} // casual
