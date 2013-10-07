//## includes protected section begin [.10]

#include "broker/adminserverimplementation.h"

//## includes protected section end   [.10]

//
// xatmi
//
#include <xatmi.h>

//
// sf
//
#include "sf/server.h"
#include "sf/service.h"


//## declarations protected section begin [.20]
//## declarations protected section end   [.20]


namespace local
{
   namespace
   {
      typedef casual::broker::AdminServerImplementation implementation_type;

      casual::sf::server::type server;
      casual::sf::server::implementation::type< implementation_type> implementation;
   }
}


//## declarations protected section begin [.30]
//## declarations protected section end   [.30]

extern "C"
{

namespace casual
{
namespace broker
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



void _broker_listServers( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {
   
     
      auto service_io = local::server->createService( serviceInfo);

      //
      // Instantiate and serialize input parameters
      //
            
      

      //## input protected section begin [2000.110]
      //## input protected section end   [2000.110]


      //
      // Instantiate the output parameters
      //
            

      //## output protected section begin [2000.120]
      //## output protected section end   [2000.120]


      //
      // Call the implementation
      //
      
      std::vector< admin::ServerVO> serviceReturn = service_io.call( 
         *local::implementation, 
         &local::implementation_type::_broker_listServers);
      
      
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
      local::server->handleException( serviceInfo, reply);
   }

   tpreturn(
      reply.value,
      reply.code,
      reply.data,
      reply.size,
      reply.flags);
}
	
	


void _broker_listServices( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {
   
     
      auto service_io = local::server->createService( serviceInfo);

      //
      // Instantiate and serialize input parameters
      //
            
      

      //## input protected section begin [2010.110]
      //## input protected section end   [2010.110]


      //
      // Instantiate the output parameters
      //
            

      //## output protected section begin [2010.120]
      //## output protected section end   [2010.120]


      //
      // Call the implementation
      //
      
      std::vector< admin::ServiceVO> serviceReturn = service_io.call( 
         *local::implementation, 
         &local::implementation_type::_broker_listServices);
      
      
      //
      // Serialize output
      //
            
      service_io << CASUAL_MAKE_NVP( serviceReturn);

      //## output protected section begin [2010.200]
      //## output protected section end   [2010.200]

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
	

void _broker_updateInstances( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {


      auto service_io = local::server->createService( serviceInfo);

      //
      // Instantiate and serialize input parameters
      //



      //## input protected section begin [2010.110]
      //## input protected section end   [2010.110]


      //
      // Instantiate the output parameters
      //


      //## output protected section begin [2010.120]
      //## output protected section end   [2010.120]


      //
      // Call the implementation
      //

      std::vector< admin::ServiceVO> serviceReturn = service_io.call(
         *local::implementation,
         &local::implementation_type::_broker_listServices);


      //
      // Serialize output
      //

      service_io << CASUAL_MAKE_NVP( serviceReturn);

      //## output protected section begin [2010.200]
      //## output protected section end   [2010.200]

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

	

} // broker
} // casual


} // extern "C"

