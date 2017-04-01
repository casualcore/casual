//!
//! casual
//!


#include "transaction/manager/admin/server.h"
#include "transaction/manager/admin/transform.h"
#include "transaction/manager/state.h"
#include "transaction/manager/manager.h"
#include "transaction/manager/action.h"

//
// xatmi
//
#include <xatmi.h>

//
// sf
//
#include "sf/server.h"
#include "sf/service/interface.h"



namespace casual
{
   namespace transaction
   {
      namespace admin
      {
         namespace local
         {
            namespace
            {
               casual::sf::Server server;
            }
         }

         void transaction_state( TPSVCINFO *information, State& state)
         {
            casual::sf::service::reply::State reply;

            try
            {
               auto service_io = local::server.service( *information);

               auto serviceReturn = service_io.call( &transform::state, state);

               service_io << CASUAL_MAKE_NVP( serviceReturn);

               reply = service_io.finalize();
            }
            catch( ...)
            {
               local::server.exception( *information, reply);
            }

            tpreturn(
               reply.value,
               reply.code,
               reply.data,
               reply.size,
               reply.flags);
         }

         void update_instances( TPSVCINFO *information, State& state)
         {
            casual::sf::service::reply::State reply;

            try
            {
               auto service_io = local::server.service( *information);

               std::vector< vo::update::Instances> instances;

               service_io >> CASUAL_MAKE_NVP( instances);

               auto serviceReturn = service_io.call( &action::resource::insances, state, std::move( instances));

               service_io << CASUAL_MAKE_NVP( serviceReturn);

               reply = service_io.finalize();
            }
            catch( ...)
            {
               local::server.exception( *information, reply);
            }

            tpreturn(
               reply.value,
               reply.code,
               reply.data,
               reply.size,
               reply.flags);
         }


         common::server::Arguments services( State& state)
         {
            common::server::Arguments result{ { common::process::path()}, nullptr, nullptr};

            result.services.emplace_back( ".casual.transaction.state",
                  std::bind( &transaction_state, std::placeholders::_1, std::ref( state)),
                  common::service::category::admin, common::service::transaction::Type::none);

            result.services.emplace_back( ".casual.transaction.update.instances",
                  std::bind( &update_instances, std::placeholders::_1, std::ref( state)),
                  common::service::category::admin, common::service::transaction::Type::none);

            return result;
         }

      } // admin
   } // transaction
} // casual
