

//## includes protected section begin [.10]

#include "broker/admin/server.h"
#include "broker/admin/transformation.h"


#include "broker/broker.h"


#include "common/internal/trace.h"
#include "common/algorithm.h"

#include "sf/server.h"



namespace local
{
   namespace
   {
      typedef casual::broker::admin::Server implementation_type;

      casual::sf::server::implementation::type< implementation_type> implementation;

      casual::sf::server::type server;
   }
}


namespace casual
{
namespace broker
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

   namespace admin
   {

      extern "C"
      {
         void listServers_( TPSVCINFO *serviceInfo)
         {
            casual::sf::service::reply::State reply;

            try
            {
               auto service_io = local::server->createService( serviceInfo);

               std::vector< admin::ServerVO> serviceReturn = service_io.call(
                  *local::implementation,
                  &Server::listServers);

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

         void listServices_( TPSVCINFO *serviceInfo)
         {
            casual::sf::service::reply::State reply;

            try
            {

               auto service_io = local::server->createService( serviceInfo);


               std::vector< admin::ServiceVO> serviceReturn = service_io.call(
                  *local::implementation,
                  &Server::listServices);

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

         void updateInstances_( TPSVCINFO *serviceInfo)
         {
            casual::sf::service::reply::State reply;

            try
            {

               auto service_io = local::server->createService( serviceInfo);


               std::vector<admin::update::InstancesVO> instances;

               service_io >> CASUAL_MAKE_NVP( instances);


               service_io.call(
                  *local::implementation,
                  &Server::updateInstances,
                  instances);

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


      Server::Server( int argc, char **argv)
      {

      }



      common::server::Arguments Server::services( Broker& broker)
      {
         m_broker = &broker;

         common::server::Arguments result{ { common::process::path()}};

         result.services.emplace_back( "_broker_listServers", &listServers_, 10, common::server::Service::cNone);
         result.services.emplace_back( "_broker_listServices", &listServices_, 10, common::server::Service::cNone);
         result.services.emplace_back( "_broker_updateInstances", &updateInstances_, 10, common::server::Service::cNone);

         return result;
      }


      std::vector< admin::ServerVO> Server::listServers( )
      {

         auto& state = m_broker->state();

         std::vector< admin::ServerVO> result;

         common::range::transform( state.servers, result,
            common::chain::Nested::link(
               admin::transform::Server( state),
               common::extract::Second()));

         return result;
      }

      std::vector< admin::ServiceVO> Server::listServices( )
      {

         auto& state = m_broker->state();

         std::vector< admin::ServiceVO> result;

         common::range::transform( state.services, result,
               common::chain::Nested::link(
                  admin::transform::Service(),
                  common::extract::Second()));

         return result;
      }


      void Server::updateInstances( const std::vector<admin::update::InstancesVO>& instances)
      {
         common::trace::internal::Scope trace( "Server::_broker_updateInstances");

         m_broker->serverInstances( instances);
      }


      Broker* Server::m_broker = nullptr;

   } // admin

} // broker
} // casual



