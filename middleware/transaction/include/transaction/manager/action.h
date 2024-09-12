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
      namespace resource::proxy
      {
         namespace scale
         {
            void instances( State& state, state::resource::Proxy& proxy);
            inline auto instances( State& state) { return [ &state]( auto& proxy){ scale::instances( state, proxy);};}
         } // scale

         std::vector< admin::model::resource::Proxy> instances( State& state, std::vector< admin::model::scale::resource::proxy::Instances> instances);

      } // resource::proxy
   } // transaction::manager::action
} // casual


