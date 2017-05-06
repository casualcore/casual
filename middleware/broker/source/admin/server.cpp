//!
//!  casual
//!

#include "broker/admin/server.h"
#include "broker/transform.h"

#include "broker/broker.h"


#include "common/algorithm.h"

#include "sf/server.h"


#include "xatmi.h"


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

            sf::Server server;
         }
      }



      void service_broker_state( TPSVCINFO *serviceInfo, broker::State& state)
      {
         casual::sf::service::reply::State reply;

         try
         {

            auto service_io = local::server.service( *serviceInfo);


            auto serviceReturn = service_io.call( &transform::state, state);

            service_io << CASUAL_MAKE_NVP( serviceReturn);

            reply = service_io.finalize();
         }
         catch( ...)
         {
            local::server.exception( *serviceInfo, reply);
         }

         tpreturn(
            reply.value,
            reply.code,
            reply.data,
            reply.size,
            reply.flags);
      }



      common::server::Arguments services( broker::State& state)
      {

         common::server::Arguments result{ { common::process::path()}, nullptr, nullptr};

         result.services = {
               common::server::xatmi::service( service::name::state(),
                  std::bind( &service_broker_state, std::placeholders::_1, std::ref( state)),
                  common::service::transaction::Type::none,
                  common::service::category::admin)
         };

         return result;
      }


   } // admin

} // broker
} // casual



