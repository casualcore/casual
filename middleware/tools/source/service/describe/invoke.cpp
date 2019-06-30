//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/service/describe/invoke.h"
#include "tools/common.h"


#include "serviceframework/service/protocol/call.h"


#include "common/service/header.h"


namespace casual
{
   using namespace common;

   namespace tools
   {
      namespace service
      {
         namespace describe
         {
            std::vector< serviceframework::service::Model> invoke( const std::vector< std::string>& services)
            {
               Trace trace{ "tools::service::describe::incoke"};

               //
               // Set header so we invoke the servcie-describe protocol
               //
               common::service::header::fields()[ "casual-service-describe"] = "true";

               return algorithm::transform( services, []( const std::string& service){
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( service);

                  serviceframework::service::Model model;

                  reply >> CASUAL_NAMED_VALUE( model);

                  return model;
               });
            }

         } // describe
      } // service
   } // tools
} // casual
