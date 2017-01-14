//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_TRANSFORM_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_TRANSFORM_H_

#include "gateway/manager/state.h"

#include "gateway/manager/admin/vo.h"
#include "common/message/domain.h"


namespace casual
{


   namespace gateway
   {
      namespace transform
      {

         manager::State state( const common::message::domain::configuration::Domain& configuration);

         manager::admin::vo::State state( const manager::State& state);


      } // transform

   } // gateway


} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_TRANSFORM_H_
