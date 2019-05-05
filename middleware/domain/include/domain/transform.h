//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "domain/manager/admin/vo.h"
#include "domain/manager/state.h"

#include "configuration/domain.h"

namespace casual
{
   namespace domain
   {
      namespace transform
      {
         manager::admin::vo::State state( const manager::State& state);
         manager::State state( casual::configuration::domain::Manager domain);

         namespace environment
         {
            std::vector< common::environment::Variable> variables( const casual::configuration::Environment& environment);
         } // environment

      } // transform
   } // domain
} // casual


