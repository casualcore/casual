


#include "transaction/manager/admin/server.h"
#include "transaction/manager/admin/transform.h"
#include "transaction/manager/state.h"
#include "transaction/manager/manager.h"

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
               casual::sf::server::type server;
            }
         }

         int tpsvrinit(int argc, char **argv)
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
            casual::sf::server::sink( local::server);
         }


         void transaction_state( TPSVCINFO *serviceInfo, State& state)
         {
            casual::sf::service::reply::State reply;

            try
            {
               auto service_io = local::server->createService( serviceInfo);

               auto serviceReturn = transform::state( state);

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


         common::server::Arguments services( State& state)
         {
            common::server::Arguments result{ { common::process::path()}};

            result.server_init = &tpsvrinit;
            result.server_done = &tpsvrdone;

            result.services.emplace_back( ".casual.transaction.state",
                  std::bind( &transaction_state, std::placeholders::_1, std::ref( state)),
                  common::server::Service::Type::cCasualAdmin, common::server::Service::Transaction::none);

            return result;
         }

      } // admin
   } // transaction
} // casual
