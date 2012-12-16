//!
//! xatmi_server.cpp
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#include "xatmi_server.h"


#include <vector>
#include <algorithm>


#include "common/service_context.h"
#include "common/server_context.h"
#include "common/message_dispatch.h"


#include "utility/error.h"
#include "utility/logger.h"
#include "utility/environment.h"


using namespace casual;

namespace
{
	namespace local
	{
	   namespace transform
	   {
         struct ServiceContext
         {
            common::service::Context operator () ( const casual_service_name_mapping& mapping) const
            {
               return common::service::Context( mapping.m_name, mapping.m_functionPointer);

            }
         };
	   }
	}

}


extern "C"
{
   int tpsvrinit(int argc, char **argv);

   void tpsvrdone();
}



int casual_startServer( int argc, char** argv, struct casual_service_name_mapping* mapping, size_t size)
{
   try
   {

      std::vector< std::string> arguments;

      std::copy(
         argv,
         argv + argc,
         std::back_inserter( arguments));


      utility::environment::setExecutablePath( arguments.at( 0));

      tpsvrinit( argc, argv);

      std::vector< common::service::Context> serviceContext;

      std::transform(
         mapping,
         mapping + size,
         std::back_inserter( serviceContext),
         local::transform::ServiceContext());


      //
      // Start the message-pump
      //
      common::dispatch::Handler handler;

      handler.add< common::callee::handle::Call>( serviceContext);

      common::queue::blocking::Reader queueReader( common::ipc::getReceiveQueue());

      while( true)
      {

         auto marshal = queueReader.next();

         if( ! handler.dispatch( marshal))
         {
            utility::logger::error << "message: " << marshal.type() << " not recognized - action: discard";
         }
      }
	}
	catch( ...)
	{
	   tpsvrdone();
	   return casual::utility::error::handler();
	}
	return 0;
}








