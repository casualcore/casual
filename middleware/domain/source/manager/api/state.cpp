//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/domain/manager/api/state.h"
#include "casual/domain/manager/api/internal/transform.h"

#include "domain/manager/admin/server.h"

#include "serviceframework/service/protocol/call.h"


namespace casual
{
   using namespace common;

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
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( casual::domain::manager::admin::service::name::state);

                  casual::domain::manager::admin::model::State result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return internal::transform::state( std::move( result));
               }

            } // v1
         } // api
      } // manager
   } // domain
} // casual
