//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/manager/state.h"

namespace casual
{
   namespace domain::manager::state::order
   {
/*
      std::vector< state::dependency::Group> boot(
         const State& state,
         const std::vector< Server>& servers, 
         const std::vector< Executable>& executables);
         */

      std::vector< state::dependency::Group> boot( const State& state);

      std::vector< state::dependency::Group> shutdown( const State& state);

   } // domain::manager::state::order
} // casual