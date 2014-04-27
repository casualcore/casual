//!
//! resource_proxy_server.cpp
//!
//! Created on: Aug 2, 2013
//!     Author: Lazan
//!


#include "transaction/resource/proxy_server.h"
#include "transaction/resource/proxy.h"

#include "common/error.h"
#include "common/arguments.h"
#include "common/environment.h"
#include "common/trace.h"
#include "common/internal/trace.h"

#include "sf/log.h"






int casual_start_reource_proxy( struct casual_resource_proxy_service_argument* serverArguments)
{
   try
   {

      casual::common::trace::internal::Scope trace{ "casual_start_reource_proxy"};

      casual::transaction::resource::State state;
      state.xaSwitches = serverArguments->xaSwitches;

      {
         casual::common::Arguments arguments;

         arguments.add(
            casual::common::argument::directive( {"-q", "--tm-queue"}, "tm queue", state.tm_queue),
            casual::common::argument::directive( {"-k", "--rm-key"}, "resource key", state.rm_key),
            casual::common::argument::directive( {"-o", "--rm-openinfo"}, "open info", state.rm_openinfo),
            casual::common::argument::directive( {"-c", "--rm-closeinfo"}, "close info", state.rm_closeinfo),
            casual::common::argument::directive( {"-i", "--rm-id"}, "resource id", state.rm_id),
            casual::common::argument::directive( {"-d", "--domain"}, "domain name", state.domain)
         );

         arguments.parse( serverArguments->argc, serverArguments->argv);

         casual::common::process::path( arguments.processName());
         casual::common::environment::domain::name( state.domain);
      }

      casual::common::log::internal::transaction << CASUAL_MAKE_NVP( state);


      casual::transaction::resource::Proxy proxy( std::move( state));
      proxy.start();
   }
   catch( ...)
   {
      return casual::common::error::handler();
   }

   return 0;

}
