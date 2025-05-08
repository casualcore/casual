//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/manager/admin/server.h"
#include "transaction/manager/admin/transform.h"
#include "transaction/manager/state.h"
#include "transaction/manager/action.h"

#include "casual/manager/service/protocol.h"

namespace casual
{
   namespace transaction
   {
      namespace manager
      {
         namespace admin
         {
            namespace local
            {
               namespace
               {

                  auto state( manager::State& state)
                  {
                     return [ &state]( casual::manager::service::invoke::Parameter&& parameter)
                     {
                        return casual::manager::service::protocol::dispatch( 
                           std::move( parameter),
                           &transform::state, state);
                     };
                  }

                  namespace scale::resource::proxy
                  {
                     auto instances( manager::State& state)
                     {
                        return [ &state]( casual::manager::service::invoke::Parameter&& parameter)
                        {
                           auto protocol = casual::manager::service::protocol::deduce( std::move( parameter));
                           auto instances = protocol.extract< std::vector< admin::model::scale::resource::proxy::Instances>>( "instances");

                           return casual::manager::service::protocol::dispatch(
                              std::move( protocol),
                              &action::resource::proxy::instances,
                              state,
                              std::move( instances));
                        };
                     }
                  } // scale::resource::proxy
               } // <unnamed>
            } // local


            std::vector< casual::manager::Service> services( manager::State& state)
            {
               return { 
                     { 
                        .name = std::string{ service::name::state},
                        .function = local::state( state),
                        .visibility = common::service::visibility::Type::undiscoverable,
                        .category = std::string{ common::service::category::admin}
                     },
                     {
                        .name = std::string{ service::name::scale::resource::proxies},
                        .function = local::scale::resource::proxy::instances( state),
                        .visibility = common::service::visibility::Type::undiscoverable,
                        .category = std::string{ common::service::category::admin}
                     }
                     ,
                     // deprecated
                     { 
                        .name = ".casual/transaction/scale/instances",
                        .function = local::scale::resource::proxy::instances( state),
                        .visibility = common::service::visibility::Type::undiscoverable,
                        .category = std::string{ common::service::category::deprecated}
                     }
               };
            }

         } // admin
      } // manager
   } // transaction
} // casual
