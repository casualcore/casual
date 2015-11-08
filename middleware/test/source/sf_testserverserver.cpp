//## includes protected section begin [.10]

#include "sf_testserverimplementation.h"

#include "sf_testvo.h"

//## includes protected section end   [.10]

//
// xatmi
//
#include <xatmi.h>

//
// sf
//
#include "sf/server.h"
#include "sf/service/interface.h"


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]


namespace local
{
   namespace
   {
      typedef casual::test::TestServerImplementation implementation_type;

      casual::sf::server::type server;
      casual::sf::server::implementation::type< implementation_type> implementation;
   }
}


extern "C"
{

namespace casual
{
namespace test
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
   //
   // delete the implementation an server implementation
   //
   casual::sf::server::sink( local::implementation);
   casual::sf::server::sink( local::server);
}

//
// Services provided
//



void casual_sf_test1( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {
   
     
      auto service_io = local::server->createService( serviceInfo);

      //
      // Instantiate and serialize input parameters
      //
            
      std::vector< vo::TestVO> values;
      
      service_io >> CASUAL_MAKE_NVP( values);

      //## input protected section begin [501.110]
      //## input protected section end   [501.110]


      //
      // Instantiate the output parameters
      //
            
      std::vector< vo::TestVO> outputValues;

      //## output protected section begin [501.120]
      //## output protected section end   [501.120]


      //
      // Call the implementation
      //
      
      bool serviceReturn = service_io.call( 
         *local::implementation, 
         &local::implementation_type::casual_sf_test1, 
         values,
         outputValues);
      
      
      //
      // Serialize output
      //
            
      service_io << CASUAL_MAKE_NVP( serviceReturn);
      service_io << CASUAL_MAKE_NVP( outputValues);

      //## output protected section begin [501.200]
      //## output protected section end   [501.200]

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
	
	


void casual_sf_test2( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {
   
     
      auto service_io = local::server->createService( serviceInfo);

      //
      // Instantiate and serialize input parameters
      //
            
      bool someValue;
      
      service_io >> CASUAL_MAKE_NVP( someValue);

      //## input protected section begin [502.110]
      //## input protected section end   [502.110]


      //
      // Instantiate the output parameters
      //
            

      //## output protected section begin [502.120]
      //## output protected section end   [502.120]


      //
      // Call the implementation
      //
      
      service_io.call( 
         *local::implementation, 
         &local::implementation_type::casual_sf_test2, 
         someValue);
      
      
      //
      // Serialize output
      //
            

      //## output protected section begin [502.200]
      //## output protected section end   [502.200]

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
	
	

} // test
} // casual


} // extern "C"

