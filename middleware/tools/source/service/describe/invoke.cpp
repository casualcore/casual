//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/service/describe/invoke.h"
#include "tools/common.h"

#include "service/protocol/call.h"



#include "casual/header.h"


namespace casual
{
   using namespace common;

   namespace tools::service::describe
   {
      std::vector< common::serialize::service::Model> invoke( const std::vector< std::string>& services)
      {
         Trace trace{ "tools::service::describe::invoke"};

         return algorithm::transform( services, []( const std::string& service)
         {
            auto header = casual::Header{ .fields = header::Fields{ { { "casual-service-describe", "true"}}}};

            casual::service::protocol::binary::Call call;
            auto reply = call( service, header);
            return reply.extract< common::serialize::service::Model>( "model");
         });
      }


   } // tools::service::describe
} // casual
