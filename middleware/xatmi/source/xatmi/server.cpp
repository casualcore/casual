//!
//! casual
//!

#include "xatmi/server.h"


#include "common/server/start.h"


#include "common/functional.h"
#include "common/exception/xatmi.h"
#include "common/process.h"
#include "common/event/send.h"


#include <vector>
#include <algorithm>



using namespace casual;


namespace local
{
   namespace
   {
	   namespace transform
	   {


         std::vector< common::server::argument::xatmi::Service> services( struct casual_server_argument& value)
         {
            std::vector< common::server::argument::xatmi::Service> result;

            auto service = value.services;

            for( ; service->function_pointer != nullptr; ++service)
            {
               result.emplace_back(
                  service->name,
                  service->function_pointer,
                  common::service::transaction::mode( service->transaction),
                  service->category);
            }

            return result;
         }

         std::vector< common::server::argument::transaction::Resource> resources( struct casual_server_argument& value)
         {
            std::vector< common::server::argument::transaction::Resource> result;

            auto xaSwitch = value.xa_switches;

            while( xaSwitch->xa_switch != nullptr)
            {
               result.emplace_back( xaSwitch->key, xaSwitch->xa_switch);
               ++xaSwitch;
            }

            return result;
         }

	   } // transform
	} // <unnamed>
} // local



int casual_start_server( casual_server_argument* arguments)
{
   try
   {
      common::server::start(
            local::transform::services( *arguments),
            local::transform::resources( *arguments),
            [&](){
               if( arguments->server_init)
               {
                  if( common::invoke( arguments->server_init, arguments->argc, arguments->argv) == -1)
                  {
                     common::event::error::send( "server initialize failed - action: exit");
                     throw common::exception::xatmi::invalid::Argument{ "server initialize failed"};
                  }
               }
      });

      if( arguments->server_done)
      {
         common::invoke( arguments->server_done);
      }

	}
	catch( ...)
	{
	   return static_cast< int>( casual::common::exception::xatmi::handle());
	}
	return 0;
}








