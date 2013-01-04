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



//
// Implementation
//
#include "template_sf_server_implementation.h"
#include "template_vo.h"


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
      std::unique_ptr< casual::sf::server::Interface> server;
      std::unique_ptr< test::ServerImplementation> implementation;
   }
}



int tpsvrinit(int argc, char **argv)
{
   try
   {
      local::server = casual::sf::server::create( argc, argv);

      local::implementation.reset( new test::ServerImplementation( argc, argv));
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
   local::implementation.reset();
   local::server.reset();
}


struct ReplyState
{
   int value;
   long code;
   char* data;
   long size;
   long flags;
};

void casual_sf_test1( TPSVCINFO *serviceInfo)
{
   ReplyState reply;

   try
   {
      auto service = local::server->service( serviceInfo);

      //
      // Use the helper for IO
      //
      casual::sf::service::IO io( *service);

      //
      // Initialize the input parameters to the service implementation
      //

      std::vector< test::vo::Value> inputValues;

      io >> CASUAL_MAKE_NVP( inputValues);


      //
      // Instantiate the output parameters
      //

      bool serviceReturn;

      std::vector< test::vo::Value> outputValues;


      //
      // Call the implementation
      //

      if( service->call())
      {
         try
         {
            serviceReturn = local::implementation->casual_sf_test1( inputValues, outputValues);
         }
         catch( ...)
         {
            // TODO;
         }
      }

      //
      // Serialize output
      //
      io << CASUAL_MAKE_NVP( serviceReturn);
      io << CASUAL_MAKE_NVP( outputValues);

   }
   catch( ...)
   {
      // TODO:
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



