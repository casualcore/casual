//!
//! transform.h
//!
//! Created on: Jun 14, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_TRANSACTION_MANAGER_ADMIN_TRANSFORM_H_
#define CASUAL_TRANSACTION_MANAGER_ADMIN_TRANSFORM_H_

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

#endif // TRANSFORM_H_
