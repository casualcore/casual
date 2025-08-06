//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/admin/server.h"

#include "domain/discovery/admin/transform.h"
#include "domain/discovery/common.h"

#include "casual/manager/service/protocol.h"


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
                  return [&state]( casual::manager::service::invoke::Parameter&& parameter)
                  {                    
                     return casual::manager::service::protocol::dispatch( 
                        std::move( parameter),
                        admin::transform,
                        state);
                  };
               }
               
            } // service
         } // <unnamed>
      } // local

      std::vector< casual::manager::Service> services( discovery::State& state)
      {
         Trace trace{ "domain::discovery::admin::services"};

         return {
            casual::manager::sequential::Service{  
               .name = std::string{ admin::service::name::state},
               .function = local::service::state( state),
               .visibility = common::service::visibility::Type::undiscoverable,
               .category = std::string{ common::service::category::admin}
            }
         };

      }

   } // domain::discovery::admin
} // casual
