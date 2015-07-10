

//## includes protected section begin [.10]

#include "broker/admin/server.h"
#include "broker/admin/transformation.h"


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



      void service_update_instances( TPSVCINFO *serviceInfo, broker::State& state)
      {
         casual::sf::service::reply::State reply;

         try
         {

            auto service_io = local::server->createService( serviceInfo);


            std::vector<admin::update::InstancesVO> instances;

            service_io >> CASUAL_MAKE_NVP( instances);

            broker::update::instances( state, instances);

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


      void service_shutdown( TPSVCINFO *serviceInfo, broker::State& state)
      {
         casual::sf::service::reply::State reply;

         try
         {
            auto service_io = local::server->createService( serviceInfo);

            bool broker;

            service_io >> CASUAL_MAKE_NVP( broker);

            auto serviceReturn = broker::shutdown( state, broker);

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





      void service_broker_state( TPSVCINFO *serviceInfo, broker::State& state)
      {
         casual::sf::service::reply::State reply;

         try
         {

            auto service_io = local::server->createService( serviceInfo);


            auto serviceReturn = broker_state( state);

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



      admin::StateVO broker_state( const broker::State& state)
      {
         admin::StateVO result;

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


      void test1( TPSVCINFO *serviceInfo)
      {

      }


      common::server::Arguments services( broker::State& state)
      {

         common::server::Arguments result{ { common::process::path()}};

         result.server_init = &tpsvrinit;
         result.server_done = &tpsvrdone;

         result.services.emplace_back( ".casual.broker.state", std::bind( &service_broker_state, std::placeholders::_1, std::ref( state)), common::server::Service::Type::cCasualAdmin, common::server::Service::Transaction::none);
         result.services.emplace_back( ".casual.broker.update.instances", std::bind( &service_update_instances, std::placeholders::_1, std::ref( state)), common::server::Service::Type::cCasualAdmin, common::server::Service::Transaction::none);
         result.services.emplace_back( ".casual.broker.shutdown", std::bind( &service_shutdown, std::placeholders::_1, std::ref( state)), common::server::Service::Type::cCasualAdmin, common::server::Service::Transaction::none);

         //result.services.push_back( common::server::Service{ ".casual.test", std::bind( &test1, std::placeholders::_1), common::server::Service::Type::cCasualAdmin, common::server::Service::Transaction::none});

         return result;
      }


   } // admin

} // broker
} // casual



