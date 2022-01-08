//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "transaction/manager/state.h"

#include "transaction/manager/admin/model.h"

#include <vector>

namespace casual
{
   namespace transaction::manager::action
   {
      namespace resource
      {
         namespace scale
         {
            void instances( State& state, state::resource::Proxy& proxy);
            inline auto instances( State& state) { return [ &state]( auto& proxy){ scale::instances( state, proxy);};}
         } // scale

         std::vector< admin::model::resource::Proxy> instances( State& state, std::vector< admin::model::scale::Instances> instances);

         bool request( State& state, state::pending::Request& message);

      } // resource
   } // transaction::manager::action
} // casual


