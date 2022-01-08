//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "transaction/manager/admin/model.h"
#include "transaction/manager/state.h"

namespace casual
{
   namespace transaction::manager::admin::transform
   {

      model::Metrics metrics( const state::Metrics& value);
      state::Metrics metrics( const model::Metrics& value);

      namespace resource
      {
         model::resource::Proxy proxy( const state::resource::Proxy& value);

      } // resource

      model::State state( const manager::State& state);

   } // transaction::manager::admin::transform
} // casual


