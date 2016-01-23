//!
//! xatmi_server.cpp
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#include "xatmi_server.h"


#include <vector>
#include <algorithm>


#include "common/server/handle.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/communication/ipc.h"


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
                        common::server::service::transaction::mode( service->transaction));

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
         common::server::handle::Call( common::communication::ipc::inbound::device(), local::transform::ServerArguments{}( *serverArgument)),
         common::message::handle::Shutdown{},
      };


      //
      // Start the message-pump
      //
      common::message::dispatch::pump(
            handler,
            common::communication::ipc::inbound::device(),
            common::communication::ipc::policy::Blocking{});
	}
	catch( ...)
	{
	   return casual::common::error::handler();
	}
	return 0;
}








