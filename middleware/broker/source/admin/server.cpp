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

            }
         }

         common::server::Arguments services( broker::State& state)
         {
            return { {
                  { service::name::state(),
                     std::bind( &local::state, std::placeholders::_1, std::ref( state)),
                     common::service::transaction::Type::none,
                     common::service::category::admin()
                  }
            }};
         }

      } // admin
   } // broker
} // casual



