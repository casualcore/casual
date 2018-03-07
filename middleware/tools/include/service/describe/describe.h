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
         std::vector< sf::service::Model> describe( const std::vector< std::string>& services);

      } // service



   } // tools


} // casual

#endif // CASUAL_MIDDLEWARE_TOOLS_SERVICE_INCLUDE_DESCRIBE_H_
