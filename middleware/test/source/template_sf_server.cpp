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
#include "sf/service/interface.h"

//
// std
//
#include <memory>
#include <type_traits>



//
// Implementation
//
#include "template_sf_server_implementation.h"
#include "sf_testvo.h"


extern "C"
{
   int tpsvrinit(int argc, char **argv);
   void tpsvrdone();

   void casual_sf_test1( TPSVCINFO *transb);
   void casual_sf_test2( TPSVCINFO *transb);

}

namespace local
{
   namespace
   {

      casual::sf::server::type server;

      typedef casual::test::ServerImplementation implementation_type;

      casual::sf::server::implementation::type< implementation_type> implementation;
   }
}



int tpsvrinit(int argc, char **argv)
{
   try
   {
      local::server = casual::sf::server::create( argc, argv);

      local::implementation = casual::sf::server::implementation::make< casual::test::ServerImplementation>( argc, argv);
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


void casual_sf_test1( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {
      auto service_io = local::server->createService( serviceInfo);

      //
      // Initialize the input parameters to the service implementation
      //

      //std::vector< casual::test::vo::TestVO> inputValues;

      //service_io >> CASUAL_MAKE_NVP( inputValues);


      //
      // Instantiate the output parameters
      //

      std::vector< casual::test::vo::TestVO> outputValues;


      //
      // Call the implementation
      //
      bool serviceReturn = service_io.call(
            &local::implementation_type::casual_sf_test1,
            *local::implementation,
            outputValues);

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

void casual_sf_test2( TPSVCINFO *serviceContext)
{

   //casual::utility::logger::debug << "casual_sf_test2 called";

   tpreturn( TPSUCCESS, 0, serviceContext->data, serviceContext->len, 0);
}



