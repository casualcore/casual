//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "server/service/protocol.h"

namespace casual
{
   namespace server::service::protocol
   {

      common::serialize::service::Protocol deduce( invoke::Parameter&& parameter)
      {
         return common::serialize::service::protocol::deduce( std::move( parameter.payload));
      }

   } // server::service::protocol
} // casual
