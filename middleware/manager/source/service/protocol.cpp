//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/manager/service/protocol.h"

namespace casual
{
   namespace manager::service::protocol
   {
      serviceframework::service::Protocol deduce( invoke::Parameter&& parameter)
      {
         return serviceframework::service::protocol::deduce( std::move( parameter.payload), parameter.header);
      }
      
   } // manager::service::protocol
   
} // casual