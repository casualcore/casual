//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "common/message/service.h"
#include "service/manager/admin/model.h"

#include <string>
#include <vector>

namespace casual
{
   namespace service::unittest
   {
      //! advertise `service` to service-manager as current process
      void advertise( std::vector< std::string> services);

      //! unadvertise `service` to service-manager as current process
      void unadvertise( std::vector< std::string> services);

      namespace send
      {
         //! sends ack to service-manager
         //! @{ 
         void ack( const common::message::service::call::callee::Request& request);
         void ack( const common::message::service::lookup::Reply& lookup);
         //! @}
      } // send

      manager::admin::model::State state();

      namespace fetch
      {
         constexpr auto until = common::unittest::fetch::until( &unittest::state);
      }

   
   } // common::unittest
} // casual