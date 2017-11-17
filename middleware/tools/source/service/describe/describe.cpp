//!
//! casual 
//!



#include "service/describe/describe.h"
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

         namespace local
         {
            namespace
            {

               struct Call
               {
                  sf::service::Model operator() ( const std::string& service) const
                  {
                     sf::service::protocol::binary::Call call;
                     auto reply = call( service);

                     sf::service::Model model;

                     reply >> CASUAL_MAKE_NVP( model);

                     return model;
                  }
               };
            } // <unnamed>
         } // local


         std::vector< sf::service::Model> describe( const std::vector< std::string>& services)
         {
            Trace trace{ "tools::service::describe"};

            //
            // Set header so we invoke the servcie-describe protocol
            //
            common::service::header::fields()[ "casual-service-describe"] = "true";

            return algorithm::transform( services, local::Call{});
         }

      } // service
   } // tools
} // casual
