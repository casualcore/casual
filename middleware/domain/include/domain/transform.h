//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/admin/model.h"
#include "domain/manager/state.h"

#include "configuration/domain.h"

namespace casual
{
   namespace domain
   {
      namespace transform
      {
         manager::admin::model::State state( const manager::State& state);
         manager::State state( casual::configuration::domain::Manager domain);

         namespace environment
         {
            std::vector< common::environment::Variable> variables( const casual::configuration::Environment& environment);
         } // environment

         std::vector< manager::state::Executable> executables( const std::vector< casual::configuration::Executable>& values, const std::vector< manager::state::Group>& groups);
         std::vector< manager::state::Server> executables( const std::vector< casual::configuration::Server>& values, const std::vector< manager::state::Group>& groups);

      } // transform
   } // domain
} // casual


