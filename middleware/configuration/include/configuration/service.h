//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/serialize/macro.h"
#include "casual/platform.h"
#include <optional>

namespace casual
{
   namespace configuration
   {
      namespace service
      {
         struct Default
         {
            std::string timeout = "0s";

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( timeout);
            )
         };

      } // service
      struct Service
      {
         std::string name;
         std::optional< std::string> timeout;
         std::optional< std::vector< std::string>> routes;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            CASUAL_SERIALIZE( name);
            CASUAL_SERIALIZE( timeout);
            CASUAL_SERIALIZE( routes);
         )

         Service& operator += ( const service::Default& rhs);
         friend bool operator == ( const Service& lhs, const Service& rhs);
      };
   } // configuration
} // casual


