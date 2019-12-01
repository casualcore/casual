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
   namespace transaction
   {
      namespace manager
      {
         namespace action
         {
            State state( manager::Settings settings);

            namespace resource
            {
               struct Instances : state::Base
               {
                  using state::Base::Base;

                  void operator () ( state::resource::Proxy& proxy);
               };

               std::vector< admin::model::resource::Proxy> instances( State& state, std::vector< admin::model::scale::Instances> instances);

               bool request( State& state, state::pending::Request& message);

            } // resource

         } // action
      } // manager
   } // transaction
} // casual


