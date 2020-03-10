//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "gateway/manager/admin/server.h"
#include "gateway/manager/admin/model.h"
#include "gateway/manager/handle.h"
#include "gateway/transform.h"


#include "serviceframework/service/protocol.h"


#include "xatmi.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace admin
         {
            namespace local
            {
               namespace
               {
                  namespace service
                  {
                     auto state( manager::State& state)
                     {
                        return [&state]( common::service::invoke::Parameter&& parameter)
                        {
                           return serviceframework::service::user( 
                              std::move( parameter),
                              [&state](){ return gateway::transform::state( state);});
                        };
                     }

                     auto rediscover( manager::State& state)
                     {
                        return [&state]( common::service::invoke::Parameter&& parameter)
                        {
                           return serviceframework::service::user( 
                              std::move( parameter),
                              [&state](){ return gateway::manager::handle::rediscover( state);});

                        };
                     }
                  }


               } // <unnamed>
            } // local

            common::server::Arguments services( manager::State& state)
            {
               return { {
                     { service::name::state,
                        local::service::state( state),
                        common::service::transaction::Type::none,
                        common::service::category::admin()
                     },
                     { service::name::rediscover,
                        local::service::rediscover( state),
                        common::service::transaction::Type::none,
                        common::service::category::admin()
                     }
               }};
            }
         } // admin
      } // manager
   } // gateway
} // casual
