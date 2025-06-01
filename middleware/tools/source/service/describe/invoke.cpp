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
      std::vector< server::service::Model> invoke( const std::vector< std::string>& services)
      {
         Trace trace{ "tools::service::describe::incoke"};

         // Set header so we invoke the servcie-describe protocol
         header::fields().add( casual::header::Field{  "casual-service-describe: true"});

         return algorithm::transform( services, []( const std::string& service){
            casual::service::protocol::binary::Call call;
            auto reply = call( service);
            return reply.extract< server::service::Model>( "model");
         });
      }


   } // tools::service::describe
} // casual
