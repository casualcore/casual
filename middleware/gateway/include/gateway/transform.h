//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_TRANSFORM_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_TRANSFORM_H_

#include "gateway/manager/state.h"



#include "config/gateway.h"


namespace casual
{
   namespace gateway
   {
      namespace transform
      {

         manager::State state( const config::gateway::Gateway& configuration);


      } // transform

   } // gateway


} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_TRANSFORM_H_
