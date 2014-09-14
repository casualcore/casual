//!
//! xatmi_server.cpp
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#include "xatmi_server.h"


#include <vector>
#include <algorithm>


#include "common/server_context.h"
#include "common/message/dispatch.h"


#include "common/error.h"
#include "common/process.h"


using namespace casual;


namespace local
{
   namespace
   {
	   namespace transform
	   {

         struct ServerArguments
         {
            common::server::Arguments operator ()( struct casual_server_argument& value) const
            {
               common::server::Arguments result( value.argc, value.argv);

               auto service = value.services;

               while( service->functionPointer != nullptr)
               {
                  result.services.emplace_back(
                        service->name,
                        service->functionPointer,
                        service->type,
                        common::server::Service::TransactionType( service->transaction));

                  ++service;
               }

               auto xaSwitch = value.xaSwitches;

               while( xaSwitch->xaSwitch != nullptr)
               {
                  result.resources.emplace_back( xaSwitch->key, xaSwitch->xaSwitch);
                  ++xaSwitch;
               }

               result.server_init = value.serviceInit;
               result.server_done = value.serviceDone;

               return result;

            }
         };
	   } // transform
	} // <unnamed>
} // local



int casual_start_server( casual_server_argument* serverArgument)
{
   try
   {
      common::process::path( serverArgument->argv[ 0]);

      common::message::dispatch::Handler handler{
         common::callee::handle::Call{ local::transform::ServerArguments()( *serverArgument)},
      };

      //
      // Start the message-pump
      //
      common::queue::blocking::Reader receiveQueue( common::ipc::receive::queue());
      common::message::dispatch::pump( handler, receiveQueue);
	}
   catch( const common::exception::signal::Terminate&)
   {
      return 1;
   }
	catch( ...)
	{
	   return casual::common::error::handler();
	}
	return 0;
}








