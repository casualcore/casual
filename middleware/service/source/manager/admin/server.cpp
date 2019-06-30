//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/manager/admin/server.h"
#include "service/transform.h"


#include "common/algorithm.h"

#include "serviceframework/service/protocol.h"



namespace casual
{
   namespace service
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

                  namespace metric
                  {

                     common::service::invoke::Result reset( common::service::invoke::Parameter&& parameter, manager::State& state)
                     {
                        auto protocol = serviceframework::service::protocol::deduce( std::move( parameter));

                        std::vector< std::string> services;
                        protocol >> CASUAL_NAMED_VALUE( services);

                        auto result = serviceframework::service::user( protocol, &manager::State::metric_reset, state, std::move( services));

                        protocol << CASUAL_NAMED_VALUE( result);

                        return protocol.finalize();
                     }

                  } // metric

               }
            }

            common::server::Arguments services( manager::State& state)
            {
               return { {
                     { service::name::state(),
                        std::bind( &local::state, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::category::admin()
                     },
                     { service::name::metric::reset(),
                        std::bind( &local::metric::reset, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::category::admin()
                     }
               }};
            }

         } // admin
      } // manager
   } // service
} // casual



