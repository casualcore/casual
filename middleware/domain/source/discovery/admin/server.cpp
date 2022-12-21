//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/admin/server.h"

#include "domain/discovery/admin/transform.h"
#include "domain/discovery/common.h"

#include "serviceframework/service/protocol.h"


namespace casual
{
   namespace domain::discovery::admin
   {
      namespace local
      {
         namespace
         {
            namespace service
            {
               auto state( discovery::State& state)
               {
                  return [&state]( common::service::invoke::Parameter&& parameter)
                  {                    
                     return serviceframework::service::user( 
                        std::move( parameter),
                        admin::transform,
                        state);
                  };
               }
               
            } // service
         } // <unnamed>
      } // local

      common::server::Arguments services( discovery::State& state)
      {
         Trace trace{ "domain::discovery::admin::services"};

         return common::server::Arguments{ {
            { admin::service::name::state,
               local::service::state( state),
               common::service::transaction::Type::none,
               common::service::visibility::Type::undiscoverable,
               common::service::category::admin
            }
         }};

      }

   } // domain::discovery::admin
} // casual
