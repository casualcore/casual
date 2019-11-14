//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "casual/transaction/resource/proxy/server.h"

#include "transaction/resource/proxy.h"
#include "transaction/common.h"

#include "common/exception/handle.h"
#include "common/exception/signal.h"
#include "common/argument.h"
#include "common/environment.h"


int casual_start_reource_proxy( struct casual_resource_proxy_service_argument* serverArguments)
{
   casual::common::log::line( casual::common::log::category::error, "deprecated entry point 'casual_start_reource_proxy', please regenerate the resource proxy");
   return casual_start_resource_proxy( serverArguments);
}

int casual_start_resource_proxy( struct casual_resource_proxy_service_argument* serverArguments)
{
   return casual::common::exception::guard( [&]()
   {
      casual::transaction::Trace trace{ "casual_start_resource_proxy"};

      casual::transaction::resource::proxy::Settings settings;

      {
         using namespace casual::common::argument;
         Parse{ "resource proxy server",
            Option( std::tie( settings.key), {"-k", "--rm-key"}, "resource key"),
            Option( std::tie( settings.openinfo), {"-o", "--rm-openinfo"}, "open info"),
            Option( std::tie( settings.closeinfo), {"-c", "--rm-closeinfo"}, "close info"),
            Option( std::tie( settings.id), {"-i", "--rm-id"}, "resource id")
         }( serverArguments->argc, serverArguments->argv);
      }

      casual::transaction::resource::Proxy proxy( std::move( settings), serverArguments->xaSwitches);
      proxy.start();
   });

}
