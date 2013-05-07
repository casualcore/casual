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


#include "common/error.h"
#include "common/logger.h"
#include "common/environment.h"


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

         struct ServerArguments
         {
            common::server::Arguments operator ()( struct casual_server_argument& value) const
            {
               common::server::Arguments result;

               std::transform(
                     value.m_serverStart,
                     value.m_serverEnd,
                     std::back_inserter( result.m_services),
                     transform::ServiceContext());

               result.m_argc = value.m_argc;
               result.m_argv = value.m_argv;

               result.m_server_init = value.m_serviceInit;
               result.m_server_done = value.m_serviceDone;

               return result;

            }
         };
	   }
	}

}

/*
int casual_initialize_server( int argc, char** argv, struct casual_service_name_mapping* mapping, size_t size)
{
   try
   {

      std::vector< std::string> arguments;

      std::copy(
         argv,
         argv + argc,
         std::back_inserter( arguments));


      utility::environment::setExecutablePath( arguments.at( 0));

      std::vector< common::service::Context> serviceContext;

      std::transform(
        mapping,
        mapping + size,
        std::back_inserter( serviceContext),
        local::transform::ServiceContext());


      common::server::Context::instance().initializeServer( serviceContext);

   }
   catch( ...)
   {
      return casual::utility::error::handler();
   }
   return 0;
}
*/


int casual_start_server( casual_server_argument* serverArgument)
{
   try
   {


     common::server::Arguments arguments = local::transform::ServerArguments()( *serverArgument);


      common::environment::setExecutablePath( serverArgument->m_argv[ 0]);


      //
      // Start the message-pump
      //
      common::dispatch::Handler handler;

      handler.add< common::callee::handle::Call>( arguments);

      common::queue::blocking::Reader queueReader( common::ipc::getReceiveQueue());

      while( true)
      {

         auto marshal = queueReader.next();

         if( ! handler.dispatch( marshal))
         {
            common::logger::error << "message: " << marshal.type() << " not recognized - action: discard";
         }
      }
	}
	catch( ...)
	{
	   tpsvrdone();
	   return casual::common::error::handler();
	}
	return 0;
}








