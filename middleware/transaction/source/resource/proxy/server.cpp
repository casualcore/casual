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

      using namespace casual::common::argument;
      
      Parse{ "resource proxy server",
         Option{ std::tie( settings.id), { "--id"}, "resource id "}( cardinality::one{})
      }( serverArguments->argc, serverArguments->argv);


      casual::transaction::resource::Proxy{ std::move( settings), serverArguments->xaSwitches}.start();
   });

}
