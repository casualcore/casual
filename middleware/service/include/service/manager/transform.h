//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "service/manager/state.h"
#include "service/manager/admin/model.h"

#include "configuration/model.h"


namespace casual
{
   namespace service::manager::transform
   {

      manager::admin::model::State state( const manager::State& state);

      casual::configuration::model::service::Model configuration( const manager::State& state);

   } // service::manager::transform
} // casual


