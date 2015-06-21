


#include "transaction/manager/admin/server.h"
#include "transaction/manager/admin/transform.h"
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



namespace local
{
   namespace
   {
      typedef casual::transaction::admin::Server implementation_type;

      casual::sf::server::type server;
      casual::sf::server::implementation::type< implementation_type> implementation;
   }
}


extern "C"
{

int tpsvrinit(int argc, char **argv)
{
   try
   {
      local::server = casual::sf::server::create( argc, argv);

      local::implementation = casual::sf::server::implementation::make< local::implementation_type>( argc, argv);
   }
   catch( ...)
   {
      // TODO
   }

   return 0;
}

void tpsvrdone()
{

   casual::sf::server::sink( local::implementation);
   casual::sf::server::sink( local::server);
}


void transaction_state( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {
      auto service_io = local::server->createService( serviceInfo);

      auto serviceReturn = service_io.call(
         *local::implementation,
         &local::implementation_type::state);

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


} // extern "C"

namespace casual
{
   namespace transaction
   {
      namespace admin
      {

         common::server::Arguments Server::services( Manager& manager)
         {
            m_manager = &manager;

            common::server::Arguments result{ { common::process::path()}};

            result.services.emplace_back( ".casual.transaction.state", &transaction_state, common::server::Service::Type::cCasualAdmin, common::server::Service::Transaction::none);

            return result;
         }

         Server::Server( int argc, char **argv)
         {

         }

         Server::~Server()
         {

         }




         vo::State Server::state()
         {
            return transform::state( m_manager->state());
         }

         Manager* Server::m_manager = nullptr;


      } // admin
   } // transaction
} // casual
