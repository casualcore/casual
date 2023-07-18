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

      namespace detail::range
      {
         using Executables = common::range::type_t< const std::vector< casual::configuration::model::domain::Executable>>;
         using Servers = common::range::type_t< const std::vector< casual::configuration::model::domain::Server>>;
      } // detail::range

      void modify( const casual::configuration::model::domain::Executable& source, manager::state::Executable& target, const std::vector< manager::state::Group>& groups);
      void modify( const casual::configuration::model::domain::Server& source, manager::state::Server& target, const std::vector< manager::state::Group>& groups);

      std::vector< manager::state::Executable> alias( const std::vector< casual::configuration::model::domain::Executable>& values, const std::vector< manager::state::Group>& groups);
      std::vector< manager::state::Server> alias( const std::vector< casual::configuration::model::domain::Server>& values, const std::vector< manager::state::Group>& groups);

   } // domain::manager::transform
} // casual


