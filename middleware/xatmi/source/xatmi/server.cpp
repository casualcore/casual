//!
//! casual
//!

#include "xatmi/server.h"


#include <vector>
#include <algorithm>


#include "common/server/handle/call.h"
#include "common/server/handle/conversation.h"
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
            common::server::Arguments result( value.argc, value.argv, value.service_init, value.service_done);

            auto service = value.services;

            for( ; service->function_pointer != nullptr; ++service)
            {
               result.services.push_back(
                     common::server::xatmi::service(
                        service->name,
                        service->function_pointer,
                        common::service::transaction::mode( service->transaction),
                        service->category));
            }

            auto xaSwitch = value.xa_switches;

            while( xaSwitch->xa_switch != nullptr)
            {
               result.resources.emplace_back( xaSwitch->key, xaSwitch->xa_switch);
               ++xaSwitch;
            }

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
         common::server::handle::Conversation{},
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








