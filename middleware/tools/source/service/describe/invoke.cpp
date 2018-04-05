//!
//! casual 
//!



#include "tools/service/describe/invoke.h"
#include "common.h"


#include "sf/service/protocol/call.h"


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
            std::vector< sf::service::Model> invoke( const std::vector< std::string>& services)
            {
               Trace trace{ "tools::service::describe::incoke"};

               //
               // Set header so we invoke the servcie-describe protocol
               //
               common::service::header::fields()[ "casual-service-describe"] = "true";

               return algorithm::transform( services, []( const std::string& service){
                  sf::service::protocol::binary::Call call;
                  auto reply = call( service);

                  sf::service::Model model;

                  reply >> CASUAL_MAKE_NVP( model);

                  return model;
               });
            }

         } // describe
      } // service
   } // tools
} // casual
