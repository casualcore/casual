//!
//! casual
//!

#include "queue/broker/admin/server.h"

#include "queue/broker/broker.h"
#include "queue/common/transform.h"


#include "sf/server.h"

namespace local
{
   namespace
   {
      casual::sf::server::type server;
   }
}

namespace casual
{
   namespace queue
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



      namespace broker
      {
         namespace admin
         {

            namespace service
            {
               extern "C"
               {

                  void list_queues( TPSVCINFO *serviceInfo, broker::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        auto service_io = local::server->createService( serviceInfo);


                        auto serviceReturn = service_io.call( &admin::list_queues, state);

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


                  void list_messages( TPSVCINFO *serviceInfo, broker::State& state)
                  {
                     casual::sf::service::reply::State reply;

                     try
                     {
                        auto service_io = local::server->createService( serviceInfo);

                        std::string queue;
                        service_io >> CASUAL_MAKE_NVP( queue);

                        auto serviceReturn = service_io.call( &admin::list_messages, state, queue);

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

            common::server::Arguments services( broker::State& state)
            {
               common::server::Arguments result{ { common::process::path()}};

               result.services.emplace_back( ".casual.queue.list.queues",
                     std::bind( &service::list_queues, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::service::transaction::Type::none);

               result.services.emplace_back( ".casual.queue.list.messages",
                     std::bind( &service::list_messages, std::placeholders::_1, std::ref( state)),
                     common::server::Service::Type::cCasualAdmin,
                     common::service::transaction::Type::none);

               return result;
            }
         } // admin
      } // broker
   } // queue


} // casual
