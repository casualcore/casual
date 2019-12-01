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
   namespace transaction
   {
      namespace manager
      {
         namespace admin
         {    
            namespace transform
            {
               model::Metrics metrics( const state::Metrics& value);
               state::Metrics metrics( const model::Metrics& value);

               namespace resource
               {
                  struct Instance
                  {
                     model::resource::Instance operator () ( const state::resource::Proxy::Instance& value) const;
                  };

                  struct Proxy
                  {
                     model::resource::Proxy operator () ( const state::resource::Proxy& value) const;
                  };

               } // resource

               model::State state( const manager::State& state);

            } // transform
         } // admin
      } // manager
   } // transaction
} // casual


