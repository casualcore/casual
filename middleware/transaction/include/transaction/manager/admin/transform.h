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
      namespace transform
      {

         struct Statistics
         {
            vo::Statistics operator () ( const state::Statistics& value) const;
         };

         struct Stats
         {
            vo::Stats operator () ( const state::Stats& value) const;
         };

         namespace resource
         {
            struct Instance
            {
               vo::resource::Instance operator () ( const state::resource::Proxy::Instance& value) const;
            };

            struct Proxy
            {
               vo::resource::Proxy operator () ( const state::resource::Proxy& value) const;

            };

         } // resource

         vo::State state( const State& state);


      } // transform

   } // transaction



} // casual


