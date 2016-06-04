//!
//! casual
//!


#include "gateway/manager/admin/server.h"
#include "gateway/manager/admin/vo.h"
#include "gateway/transform.h"


#include "sf/server.h"



namespace casual
{
   namespace local
   {
      namespace
      {
         sf::server::type server;
      }
   }

   namespace gateway
   {
      extern "C"
      {
         int tpsvrinit( int argc, char **argv)
         {
            try
            {
               local::server = casual::sf::server::create( argc, argv);
            }
            catch( ...)
            {
               // TODO
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
      }



      namespace manager
      {
         namespace admin
         {

            namespace service
            {
               extern "C"
               {

                  void get_state( TPSVCINFO *serviceInfo, manager::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        auto service_io = local::server->createService( serviceInfo);

                        auto serviceReturn = gateway::transform::state( state);

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

               }
            } // service


            common::server::Arguments services( manager::State& state)
            {
               common::server::Arguments result{ { common::process::path()}};

               result.services.emplace_back( ".casual.gateway.state",
                     std::bind( &service::get_state, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::server::Service::Transaction::none);


               return result;
            }
         } // admin
      } // manager
   } // gateway
} // casual
