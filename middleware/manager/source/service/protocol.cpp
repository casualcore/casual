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
      common::serialize::service::Protocol deduce( invoke::Parameter&& parameter)
      {
         return common::serialize::service::protocol::deduce( std::move( parameter.payload), parameter.header);
      }

      Concurrent deduce( invoke::concurrent::Parameter&& parameter)
      {
         return Concurrent{
            .protocol = common::serialize::service::protocol::deduce( std::move( parameter.invoke.payload), parameter.invoke.header),
            .callback = std::move( parameter.callback)
         };
      }
      
   } // manager::service::protocol
   
} // casual
