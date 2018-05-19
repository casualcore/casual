//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


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


