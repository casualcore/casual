//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "gateway/manager/admin/server.h"
#include "gateway/manager/admin/vo.h"
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

                     common::service::invoke::Result state( common::service::invoke::Parameter&& parameter, manager::State& state)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                        manager::admin::vo::State (*function)( const manager::State& state) = &gateway::transform::state;

                        auto result = serviceframework::service::user( protocol, function, state);

                        protocol << CASUAL_NAMED_VALUE( result);
                        return protocol.finalize();
                     }


                  }
               } // <unnamed>
            } // local


            common::server::Arguments services( manager::State& state)
            {
               return { {
                     { service::name::state(),
                        std::bind( &local::service::state, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::category::admin()
                     }
               }};
            }
         } // admin
      } // manager
   } // gateway
} // casual
