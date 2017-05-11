//!
//! casual
//!

#include "xatmi/server.h"




#include "common/server/start.h"


#include "common/error.h"
#include "common/functional.h"
#include "common/process.h"


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
      common::process::path( arguments->argv[ 0]);

      common::server::start(
            local::transform::services( *arguments),
            local::transform::resources( *arguments),
            [&](){
               if( arguments->server_init)
               {
                  if( common::invoke( arguments->server_init, arguments->argc, arguments->argv) == -1)
                  {
                     throw common::exception::xatmi::invalid::Argument{ "server init failed"};
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
	   return casual::common::error::handler();
	}
	return 0;
}








