//!
//! main.cpp
//!
//! Created on: Nov 22, 2014
//!     Author: Lazan
//!


#include "queue/api/rm/queue.h"

#include "xatmi.h"
#include "sf/server.h"
#include "sf/service/interface.h"



namespace casual
{
   namespace queue
   {

      class Server
      {
      public:
         Server( int argc, char** argv)
         {

         }

         ~Server()
         {

         }

         common::Uuid enqueue( const std::string& queue, casual::queue::Message& message)
         {
            return queue::rm::enqueue( queue, message);

         }

         std::vector< casual::queue::Message> dequeue( const std::string& queue, const queue::Selector& selector)
         {
            return queue::rm::dequeue( queue, selector);
         }
      };


   } // queue
} // casual


namespace local
{
   namespace
   {

      casual::sf::server::type server;

      using implementation_type = casual::queue::Server;

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

         local::implementation = casual::sf::server::implementation::make< casual::queue::Server>( argc, argv);
      }
      catch( ...)
      {
         // TODO
      }

      return 0;
   }

   void tpsvrdone()
   {
      //
      // delete the implementation an server implementation
      //
      casual::sf::server::sink( local::implementation);
      casual::sf::server::sink( local::server);
   }


   void enqueue( TPSVCINFO *serviceContext)
   {
      casual::sf::service::reply::State reply;

      try
      {
         auto service_io = local::server->createService( serviceContext);

         std::string queue;
         service_io >> CASUAL_MAKE_NVP( queue);

         casual::queue::Message message;
         service_io >> CASUAL_MAKE_NVP( message);

         //
         // Call the implementation
         //
         auto returnValue = service_io.call(
               &local::implementation_type::enqueue,
               *local::implementation,
               queue, message);

         //
         // Serialize output
         //
         service_io << CASUAL_MAKE_NVP( returnValue);

         reply = service_io.finalize();
      }
      catch( ...)
      {
         local::server->handleException( serviceContext, reply);
      }

      tpreturn(
         reply.value,
         reply.code,
         reply.data,
         reply.size,
         reply.flags);
   }


   void dequeue( TPSVCINFO *serviceContext)
   {
      casual::sf::service::reply::State reply;

      try
      {
         auto service_io = local::server->createService( serviceContext);

         std::string queue;
         casual::queue::Selector selector;
         service_io >> CASUAL_MAKE_NVP( queue);
         service_io >> CASUAL_MAKE_NVP( selector);

         //
         // Call the implementation
         //
         auto returnValue = service_io.call(
            &local::implementation_type::dequeue,
            *local::implementation,
            queue, selector);

         //
         // Serialize output
         //
         service_io << CASUAL_MAKE_NVP( returnValue);

         reply = service_io.finalize();
      }
      catch( ...)
      {
         local::server->handleException( serviceContext, reply);
      }

      tpreturn(
         reply.value,
         reply.code,
         reply.data,
         reply.size,
         reply.flags);
   }
}

