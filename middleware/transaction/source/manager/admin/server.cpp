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

                  common::service::invoke::Result state( common::service::invoke::Parameter&& parameter, manager::State& state)
                  {
                     auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                     auto result = serviceframework::service::user( protocol, &transform::state, state);

                     protocol << CASUAL_NAMED_VALUE( result);
                     return protocol.finalize();
                  }

                  namespace scale
                  {
                     common::service::invoke::Result instances( common::service::invoke::Parameter&& parameter, manager::State& state)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                        std::vector< admin::model::scale::Instances> instances;
                        protocol >> CASUAL_NAMED_VALUE( instances);

                        auto result = serviceframework::service::user( protocol, &action::resource::instances, state, std::move( instances));

                        protocol << CASUAL_NAMED_VALUE( result);
                        return protocol.finalize();

                     }
                  } // update
               } // <unnamed>
            } // local


            common::server::Arguments services( manager::State& state)
            {
               return { {
                     { service::name::state,
                        std::bind( &local::state, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::visibility::Type::undiscoverable,
                        common::service::category::admin
                     },
                     { service::name::scale::instances,
                        std::bind( &local::scale::instances, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::visibility::Type::undiscoverable,
                        common::service::category::admin
                     }
               }};
            }

         } // admin
      } // manager
   } // transaction
} // casual
