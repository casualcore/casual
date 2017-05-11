//!
//! casual
//!


#include "transaction/manager/admin/server.h"
#include "transaction/manager/admin/transform.h"
#include "transaction/manager/state.h"
#include "transaction/manager/manager.h"
#include "transaction/manager/action.h"

//
// sf
//

#include "sf/service/protocol/call.h"
#include "sf/service/protocol.h"


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

                  common::service::invoke::Result state( common::service::invoke::Parameter&& parameter, State& state)
                  {
                     auto protocol = sf::service::protocol::deduce( std::move( parameter));

                     auto result = sf::service::user( protocol, &transform::state, state);

                     protocol << CASUAL_MAKE_NVP( result);
                     return protocol.finalize();
                  }

                  namespace update
                  {
                     common::service::invoke::Result instances( common::service::invoke::Parameter&& parameter, State& state)
                     {
                        auto protocol = sf::service::protocol::deduce( std::move( parameter));

                        std::vector< vo::update::Instances> instances;
                        protocol >> CASUAL_MAKE_NVP( instances);

                        auto result = sf::service::user( protocol, &action::resource::insances, state, std::move( instances));

                        protocol << CASUAL_MAKE_NVP( result);
                        return protocol.finalize();

                     }

                  } // update

               } // <unnamed>
            } // local


            common::server::Arguments services( State& state)
            {
               return { {
                     { service::name::state(),
                        std::bind( &local::state, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::category::admin()
                     },
                     { service::name::update::instances(),
                        std::bind( &local::update::instances, std::placeholders::_1, std::ref( state)),
                        common::service::transaction::Type::none,
                        common::service::category::admin()
                     }
               }};
            }

         } // manager
      } // admin
   } // transaction
} // casual
