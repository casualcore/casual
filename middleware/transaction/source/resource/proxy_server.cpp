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
      casual::transaction::Trace trace{ "casual_start_resource_proxy"};

      casual::transaction::resource::proxy::Settings settings;

      {
         casual::common::Arguments arguments{ {
            casual::common::argument::directive( {"-k", "--rm-key"}, "resource key", settings.key),
            casual::common::argument::directive( {"-o", "--rm-openinfo"}, "open info", settings.openinfo),
            casual::common::argument::directive( {"-c", "--rm-closeinfo"}, "close info", settings.closeinfo),
            casual::common::argument::directive( {"-i", "--rm-id"}, "resource id", settings.id),
         }};

         arguments.parse( serverArguments->argc, serverArguments->argv);
      }

      casual::transaction::resource::Proxy proxy( std::move( settings), serverArguments->xaSwitches);
      proxy.start();
   }
   catch( const casual::common::exception::signal::Terminate& exception)
   {
   }
   catch( ...)
   {
      return casual::common::exception::handle();
   }

   return 0;

}
