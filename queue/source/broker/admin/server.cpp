//!
//! server.cpp
//!
//! Created on: Oct 2, 2014
//!     Author: Lazan
//!

#include "queue/broker/admin/server.h"

#include "queue/broker/broker.h"
#include "queue/transform.h"


#include "sf/server.h"

namespace local
{
   namespace
   {
      typedef casual::queue::broker::admin::Server implementation_type;

      casual::sf::server::implementation::type< implementation_type> implementation;

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

               local::implementation = casual::sf::server::implementation::make< local::implementation_type>( argc, argv);

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
            casual::sf::server::sink( local::implementation);
         }
      }



      namespace broker
      {
         namespace admin
         {

            extern "C"
            {
               void listGroups_( TPSVCINFO *serviceInfo)
               {
                  casual::sf::service::reply::State reply;

                  try
                  {
                     auto service_io = local::server->createService( serviceInfo);

                     std::vector< admin::GroupVO> serviceReturn = service_io.call(
                        *local::implementation,
                        &Server::listGroups);

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

               void listQueues_( TPSVCINFO *serviceInfo)
               {
                  casual::sf::service::reply::State reply;

                  try
                  {
                     auto service_io = local::server->createService( serviceInfo);

                     std::vector< std::string> groups;
                     service_io >> CASUAL_MAKE_NVP( groups);

                     auto serviceReturn = service_io.call(
                        *local::implementation,
                        &Server::listQueues, groups);

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


            common::server::Arguments Server::services( Broker& broker)
            {
               m_broker = &broker;

               common::server::Arguments result{ { common::process::path()}};

               result.services.emplace_back( ".casual.queue.list.groups", &listGroups_, 10, common::server::Service::cNone);
               result.services.emplace_back( ".casual.queue.list.queues", &listQueues_, 10, common::server::Service::cNone);

               return result;
            }

            Server::Server( int argc, char **argv)
            {

            }


            std::vector< admin::GroupVO> Server::listGroups()
            {
               return transform::groups( m_broker->state());
            }

            std::vector< admin::verbose::GroupVO> Server::listQueues( std::vector< std::string> groups)
            {
               std::vector< admin::verbose::GroupVO> result;

               common::range::transform( m_broker->queues( groups), result, transform::Group{});

               return result;
            }


            Broker* Server::m_broker = nullptr;

         } // admin
      } // broker
   } // queue


} // casual
