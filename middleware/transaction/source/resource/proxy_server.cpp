//!
//! casaul
//!


#include "transaction/resource/proxy_server.h"
#include "transaction/resource/proxy.h"
#include "transaction/common.h"

#include "common/exception/handle.h"
#include "common/exception/signal.h"
#include "common/arguments.h"
#include "common/environment.h"

#include "sf/log.h"




int casual_start_reource_proxy( struct casual_resource_proxy_service_argument* serverArguments)
{
   casual::sf::log::error << "deprecated entry point 'casual_start_reource_proxy', please regenerate the resource proxy\n";
   return casual_start_resource_proxy( serverArguments);
}

int casual_start_resource_proxy( struct casual_resource_proxy_service_argument* serverArguments)
{
   try
   {

      casual::transaction::Trace trace{ "casual_start_reource_proxy"};

      casual::transaction::resource::State state;
      state.xa_switches = serverArguments->xaSwitches;

      {
         casual::common::Arguments arguments{ {
            casual::common::argument::directive( {"-k", "--rm-key"}, "resource key", state.rm_key),
            casual::common::argument::directive( {"-o", "--rm-openinfo"}, "open info", state.rm_openinfo),
            casual::common::argument::directive( {"-c", "--rm-closeinfo"}, "close info", state.rm_closeinfo),
            casual::common::argument::directive( {"-i", "--rm-id"}, "resource id", state.rm_id),
         }};

         arguments.parse( serverArguments->argc, serverArguments->argv);
      }

      casual::transaction::log << CASUAL_MAKE_NVP( state);


      casual::transaction::resource::Proxy proxy( std::move( state));
      proxy.start();
   }
   catch( const casual::common::exception::signal::Terminate& exception)
   {
      return 0;
   }
   catch( ...)
   {
      return casual::common::exception::handle();
   }

   return 0;

}
