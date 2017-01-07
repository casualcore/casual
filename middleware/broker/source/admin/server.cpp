//!
//!  casual
//!

#include "broker/admin/server.h"
#include "broker/transform.h"

#include "broker/broker.h"


#include "common/internal/trace.h"
#include "common/algorithm.h"

#include "sf/server.h"




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

            casual::sf::server::type server;
         }
      }


      int tpsvrinit( int argc, char **argv)
      {
         try
         {
            local::server = casual::sf::server::create( argc, argv);

         }
         catch( ...)
         {
            return -1;
         }

         return 0;
      }

      void tpsvrdone()
      {
         //
         // delete the implementation an server implementation
         //
         casual::sf::server::sink( local::server);
      }



      void service_broker_state( TPSVCINFO *serviceInfo, broker::State& state)
      {
         casual::sf::service::reply::State reply;

         try
         {

            auto service_io = local::server->createService( serviceInfo);


            auto serviceReturn = service_io.call( &transform::state, state);

            service_io << CASUAL_MAKE_NVP( serviceReturn);

            reply = service_io.finalize();
         }
         catch( ...)
         {
            local::server->handleException( serviceInfo, reply);
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

         common::server::Arguments result{ { common::process::path()}};

         result.server_init = &tpsvrinit;
         result.server_done = &tpsvrdone;

         result.services.emplace_back( ".casual.broker.state",
               std::bind( &service_broker_state, std::placeholders::_1, std::ref( state)),
               common::service::category::admin,
               common::service::transaction::Type::none);

         return result;
      }


   } // admin

} // broker
} // casual



