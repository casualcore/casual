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

      serviceframework::service::Protocol deduce( invoke::Parameter&& parameter)
      {
         return serviceframework::service::protocol::deduce( std::move( parameter.payload), parameter.header);
      }

   } // server::service::protocol
} // casual
