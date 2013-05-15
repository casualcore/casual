//!
//! template_sf_server.cpp
//!
//! Created on: Jan 4, 2013
//!     Author: Lazan
//!


//
// xatmi
//
#include <xatmi.h>

//
// sf
//
#include "sf/server.h"
#include "sf/service.h"

//
// std
//
#include <memory>
#include <type_traits>



//
// Implementation
//
#include "monitor/monitor_sf_server_implementation.h"
#include "monitor/monitor_vo.h"


extern "C"
{
   int tpsvrinit(int argc, char **argv);
   void tpsvrdone();

   void getMonitorStatistics( TPSVCINFO *transb);

}

namespace local
{
   namespace
   {

      casual::sf::server::type server;
      casual::sf::server::implementation::type< casual::statistics::monitor::ServerImplementation> implementation;
   }
}



int tpsvrinit(int argc, char **argv)
{
   try
   {
      local::server = casual::sf::server::create( argc, argv);

      local::implementation = casual::sf::server::implementation::make< casual::statistics::monitor::ServerImplementation>( argc, argv);
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


void getMonitorStatistics( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {
      auto service_io = local::server->createService( serviceInfo);

      //
      // Initialize the input parameters to the service implementation
      //


      //
      // Instantiate the output parameters
      //

      bool serviceReturn;

      std::vector< casual::statistics::monitor::vo::MonitorVO> outputValues;


      //
      // Call the implementation
      //

      if( service_io.callImplementation())
      {
         try
         {
            serviceReturn = local::implementation->getMonitorStatistics( outputValues);
         }
         catch( ...)
         {
            service_io.handleException();
         }
      }

      //
      // Serialize output
      //
      service_io << CASUAL_MAKE_NVP( serviceReturn);
      service_io << CASUAL_MAKE_NVP( outputValues);

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


