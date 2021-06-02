//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/admin/model.h"
#include "domain/manager/state.h"

#include "configuration/model.h"

namespace casual
{
   namespace domain::manager::transform
   {
      manager::admin::model::State state( const manager::State& state);

      manager::State model( casual::configuration::Model domain);
      casual::configuration::Model model( const manager::State& state);


      std::vector< manager::state::Executable> alias( const std::vector< casual::configuration::model::domain::Executable>& values, const std::vector< manager::state::Group>& groups);
      std::vector< manager::state::Server> alias( const std::vector< casual::configuration::model::domain::Server>& values, const std::vector< manager::state::Group>& groups);

   } // domain::manager::transform
} // casual


