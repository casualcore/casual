//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "configuration/domain.h"
#include "service/manager/state.h"
#include "service/manager/admin/model.h"

#include "common/message/transaction.h"


namespace casual
{
   namespace service
   {
      namespace transform
      {

         struct Instance
         {
            common::process::Handle operator () ( const manager::state::instance::Sequential& value) const;
         };

         manager::admin::model::State state( const manager::State& state);

      } // transform
   } // service
} // casual


