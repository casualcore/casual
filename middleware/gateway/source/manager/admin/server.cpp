//!
//! casual
//!


#include "gateway/manager/admin/server.h"


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

                  void get_connections( TPSVCINFO *serviceInfo, manager::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        /*
                        auto service_io = local::server->createService( serviceInfo);


                        auto serviceReturn = admin::list_queues( state);

                        service_io << CASUAL_MAKE_NVP( serviceReturn);

                        reply = service_io.finalize();
                        */
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


            /*
            admin::State list_queues( broker::State& state)
            {
               admin::State result;

               result.queues = transform::queues( broker::queues( state));
               result.groups = transform::groups( state);

               return result;
            }

            std::vector< Message> list_messages( broker::State& state, const std::string& queue)
            {
               return transform::messages( broker::messages( state, queue));
            }
            */

            common::server::Arguments services( manager::State& state)
            {
               common::server::Arguments result{ { common::process::path()}};

               result.services.emplace_back( ".casual.gateway.get.connections",
                     std::bind( &service::get_connections, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::server::Service::Transaction::none);


               return result;
            }
         } // admin
      } // manager
   } // gateway
} // casual
