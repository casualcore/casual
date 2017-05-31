//!
//!  casual
//!

#include "broker/admin/server.h"
#include "broker/transform.h"

#include "broker/broker.h"


#include "common/algorithm.h"

#include "sf/service/protocol.h"



namespace casual
{
   namespace broker
   {
      namespace admin
      {
         namespace local
         {
            namespace
            {

               common::service::invoke::Result state( common::service::invoke::Parameter&& parameter, broker::State& state)
               {
                  auto protocol = sf::service::protocol::deduce( std::move( parameter));

                  auto result = sf::service::user( protocol, &transform::state, state);

                  protocol << CASUAL_MAKE_NVP( result);

                  return protocol.finalize();
               }

               namespace metric
               {

                  common::service::invoke::Result reset( common::service::invoke::Parameter&& parameter, broker::State& state)
                  {
                     auto protocol = sf::service::protocol::deduce( std::move( parameter));

                     std::vector< std::string> services;
                     protocol >> CASUAL_MAKE_NVP( services);

                     auto result = sf::service::user( protocol, &broker::State::metric_reset, state, std::move( services));

                     protocol << CASUAL_MAKE_NVP( result);

                     return protocol.finalize();
                  }

               } // metric

            }
         }

         common::server::Arguments services( broker::State& state)
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
   } // broker
} // casual



