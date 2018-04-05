//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_TOOLS_SERVICE_INCLUDE_DESCRIBE_H_
#define CASUAL_MIDDLEWARE_TOOLS_SERVICE_INCLUDE_DESCRIBE_H_

#include "sf/service/model.h"


namespace casual
{
   namespace tools
   {
      namespace service
      {
         namespace describe
         {
            std::vector< sf::service::Model> invoke( const std::vector< std::string>& services);   
         } // describe
      } // service
   } // tools
} // casual

#endif // CASUAL_MIDDLEWARE_TOOLS_SERVICE_INCLUDE_DESCRIBE_H_
