//## includes protected section begin [.10]
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
#include "transaction/manager/admin/adminimplementation.h"
//## declarations protected section end   [.20]


namespace local
{
   namespace
   {
      typedef casual::transaction::AdminImplementation implementation_type;

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
namespace transaction
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



void casual_listTransactions( TPSVCINFO *serviceInfo)
{
   casual::sf::service::reply::State reply;

   try
   {
   
     
      auto service_io = local::server->createService( serviceInfo);

      //
      // Instantiate and serialize input parameters
      //
            
      

      //## input protected section begin [300.110]
      //## input protected section end   [300.110]


      //
      // Instantiate the output parameters
      //
            

      //## output protected section begin [300.120]
      //## output protected section end   [300.120]


      //
      // Call the implementation
      //
      
      std::vector< vo::Transaction> serviceReturn = service_io.call( 
         *local::implementation, 
         &local::implementation_type::casual_listTransactions);
      
      
      //
      // Serialize output
      //
            
      service_io << CASUAL_MAKE_NVP( serviceReturn);

      //## output protected section begin [300.200]
      //## output protected section end   [300.200]

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
	
	

} // transaction
} // casual


} // extern "C"

