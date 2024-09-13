//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/manager/admin/server.h"
#include "transaction/manager/admin/transform.h"
#include "transaction/manager/state.h"
#include "transaction/manager/action.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/service/protocol.h"

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
                     return [ &state]( common::service::invoke::Parameter&& parameter)
                     {
                        return serviceframework::service::user( 
                           serviceframework::service::protocol::deduce( std::move( parameter)),
                           &transform::state, state);
                     };
                  }

                  namespace scale::resource::proxy
                  {
                     auto instances( manager::State& state)
                     {
                        return [ &state]( common::service::invoke::Parameter&& parameter)
                        {
                           auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));
                           auto instances = protocol.extract< std::vector< admin::model::scale::resource::proxy::Instances>>( "instances");

                           return serviceframework::service::user(
                              std::move( protocol),
                              &action::resource::proxy::instances,
                              state,
                              std::move( instances));
                        };
                     }
                  } // scale::resource::proxy
               } // <unnamed>
            } // local


            common::server::Arguments services( manager::State& state)
            {
               return { {
                     { service::name::state,
                        local::state( state),
                        common::service::transaction::Type::none,
                        common::service::visibility::Type::undiscoverable,
                        common::service::category::admin
                     },
                     { service::name::scale::resource::proxies,
                        local::scale::resource::proxy::instances( state),
                        common::service::transaction::Type::none,
                        common::service::visibility::Type::undiscoverable,
                        common::service::category::admin
                     },
                     // deprecated
                     { ".casual/transaction/scale/instances",
                        local::scale::resource::proxy::instances( state),
                        common::service::transaction::Type::none,
                        common::service::visibility::Type::undiscoverable,
                        common::service::category::deprecated
                     }
               }};
            }

         } // admin
      } // manager
   } // transaction
} // casual
