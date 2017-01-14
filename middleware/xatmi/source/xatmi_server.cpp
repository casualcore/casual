//!
//! casual
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
         common::server::Arguments arguments( struct casual_server_argument& value)
         {
            common::server::Arguments result( value.argc, value.argv);

            auto service = value.services;

            while( service->function_pointer != nullptr)
            {
               result.services.emplace_back(
                     service->name,
                     service->function_pointer,
                     service->category,
                     common::service::transaction::mode( service->transaction));

               ++service;
            }

            auto xaSwitch = value.xa_switches;

            while( xaSwitch->xa_switch != nullptr)
            {
               result.resources.emplace_back( xaSwitch->key, xaSwitch->xa_switch);
               ++xaSwitch;
            }

            result.server_init = value.service_init;
            result.server_done = value.service_done;

            return result;

         }
	   } // transform
	} // <unnamed>
} // local



int casual_start_server( casual_server_argument* serverArgument)
{
   try
   {
      common::process::path( serverArgument->argv[ 0]);

      //
      // Connect to domain
      //
      common::process::instance::connect();

      auto handler = common::communication::ipc::inbound::device().handler(
         common::server::handle::Call( local::transform::arguments( *serverArgument)),
         common::message::handle::Shutdown{},
         common::message::handle::ping()
      );


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








