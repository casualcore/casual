//## includes protected section begin [.10]


#include <xatmi.h>
#include "traffic/monitor/request_server_implementation.h"
#include "sf/server.h"
#include "sf/service/interface.h"


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]


namespace local
{
   namespace
   {
      typedef casual::traffic::monitor::RequestServerImplementation implementation_type;

      casual::sf::server::type server;
      casual::sf::server::implementation::type< implementation_type> implementation;
   }
}


extern "C"
{

namespace casual
{
namespace traffic
{
namespace monitor
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



void getMonitorStatistics( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {
   
     
      auto service_io = local::server->createService( serviceInfo);

      //
      // Instantiate and serialize input parameters
      //
            
      

      //## input protected section begin [2000.110]
      using namespace casual::traffic::monitor;
      //## input protected section end   [2000.110]


      //
      // Instantiate the output parameters
      //
            
      std::vector< ServiceEntryVO> outputValues;

      //## output protected section begin [2000.120]
      //## output protected section end   [2000.120]


      //
      // Call the implementation
      //
      
      bool serviceReturn = service_io.call( 
         *local::implementation, 
         &local::implementation_type::getMonitorStatistics, 
         outputValues);
      
      
      //
      // Serialize output
      //
            
      service_io << CASUAL_MAKE_NVP( serviceReturn);
      service_io << CASUAL_MAKE_NVP( outputValues);

      //## output protected section begin [2000.200]
      //## output protected section end   [2000.200]

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
	
	
} // monitor
} // traffic
} // casual


} // extern "C"

