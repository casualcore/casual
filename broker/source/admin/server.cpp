

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

         void admin_state( TPSVCINFO *serviceInfo)
         {
            casual::sf::service::reply::State reply;

            try
            {

               auto service_io = local::server->createService( serviceInfo);


               auto serviceReturn = service_io.call(
                  *local::implementation,
                  &Server::state);

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

         void admin_updateInstances( TPSVCINFO *serviceInfo)
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


         void admin_shutdown( TPSVCINFO *serviceInfo)
         {
            casual::sf::service::reply::State reply;

            try
            {
               auto service_io = local::server->createService( serviceInfo);

               bool broker;

               service_io >> CASUAL_MAKE_NVP( broker);

               auto serviceReturn = service_io.call(
                  *local::implementation,
                  &Server::shutdown,
                  broker);

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


      Server::Server( int argc, char **argv)
      {

      }



      common::server::Arguments Server::services( Broker& broker)
      {
         m_broker = &broker;

         common::server::Arguments result{ { common::process::path()}};

         result.services.emplace_back( ".casual.broker.state", &admin_state, common::server::Service::Type::cCasualAdmin, common::server::Service::Transaction::none);
         result.services.emplace_back( ".casual.broker.update.instances", &admin_updateInstances, common::server::Service::Type::cCasualAdmin, common::server::Service::Transaction::none);
         result.services.emplace_back( ".casual.broker.shutdown", &admin_shutdown, common::server::Service::Type::cCasualAdmin, common::server::Service::Transaction::none);

         return result;
      }




      admin::StateVO Server::state()
      {
         admin::StateVO result;

         auto& state = m_broker->state();

         {
            common::range::transform( state.servers, result.servers,
               common::chain::Nested::link(
                  admin::transform::Server(),
                  common::extract::Second()));
         }

         {
            common::range::transform( state.services, result.services,
                  common::chain::Nested::link(
                     admin::transform::Service(),
                     common::extract::Second()));
         }


         {
            common::range::transform( state.instances, result.instances,
                  common::chain::Nested::link(
                     admin::transform::Instance(),
                     common::extract::Second()));
         }

         {
            common::range::transform( state.pending, result.pending,
                  admin::transform::Pending{});
         }

         return result;
      }


      void Server::updateInstances( const std::vector<admin::update::InstancesVO>& instances)
      {
         common::trace::internal::Scope trace( "Server::updateInstances");

         m_broker->serverInstances( instances);
      }



      admin::ShutdownVO Server::shutdown( bool broker)
      {
         return m_broker->shutdown( broker);
      }


      Broker* Server::m_broker = nullptr;

   } // admin

} // broker
} // casual



