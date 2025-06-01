//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/domain/manager/api/state.h"
#include "casual/domain/manager/api/internal/transform.h"

#include "domain/manager/admin/server.h"
#include "domain/manager/admin/call.h"



namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace api
         {
            inline namespace v1 
            {
               Model state()
               {
                  auto result = domain::manager::admin::call::service< admin::model::State>( admin::service::name::state);

                  return internal::transform::state( std::move( result));
               }

            } // v1
         } // api
      } // manager
   } // domain
} // casual
