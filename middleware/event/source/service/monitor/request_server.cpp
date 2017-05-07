//## includes protected section begin [.10]


#include "event/service/monitor/request_server_implementation.h"



#include "sf/server.h"
#include "sf/service/interface.h"


#include <xatmi.h>

//## declarations protected section begin [.20]
//## declarations protected section end   [.20]


namespace local
{
   namespace
   {
      using implementation_type = casual::event::monitor::RequestServerImplementation;

      casual::sf::Server server;
      casual::sf::server::implementation::type< implementation_type> implementation;
   }
}


extern "C"
{

namespace casual
{
namespace event
{
namespace monitor
{



int tpsvrinit(int argc, char **argv)
{
   try
   {
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
   casual::sf::server::implementation::sink( std::move( local::implementation));
}

//
// Services provided
//



void getMonitorStatistics( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {
   
     
      auto service_io = local::server.service( *serviceInfo);

      //
      // Instantiate and serialize input parameters
      //
            
      

      //## input protected section begin [2000.110]
      using namespace casual::event::monitor;
      //## input protected section end   [2000.110]


      //## output protected section begin [2000.120]
      //## output protected section end   [2000.120]


      //
      // Call the implementation
      //
      
      auto serviceReturn = service_io.call(
         &local::implementation_type::getMonitorStatistics,
         *local::implementation);
      
      
      //
      // Serialize output
      //
      service_io << CASUAL_MAKE_NVP( serviceReturn);

      //## output protected section begin [2000.200]
      //## output protected section end   [2000.200]

      reply = service_io.finalize();
   }
   catch( ...)
   {
      local::server.exception( *serviceInfo, reply);
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

