//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "transaction/manager/admin/transactionvo.h"
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
               admin::Metrics metrics( const state::Metrics& value);
               state::Metrics metrics( const admin::Metrics& value);

               namespace resource
               {
                  struct Instance
                  {
                     admin::resource::Instance operator () ( const state::resource::Proxy::Instance& value) const;
                  };

                  struct Proxy
                  {
                     admin::resource::Proxy operator () ( const state::resource::Proxy& value) const;
                  };

               } // resource

               admin::State state( const manager::State& state);

            } // transform
         } // admin
      } // manager
   } // transaction
} // casual


