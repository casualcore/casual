//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/serialize/macro.h"
#include "common/platform.h"
#include "common/optional.h"

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
               CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( timeout));
            )
         };

      } // service
      struct Service
      {
         std::string name;
         common::optional< std::string> timeout;
         common::optional< std::vector< std::string>> routes;

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


