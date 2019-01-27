//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"

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
               archive & CASUAL_MAKE_NVP( timeout);
            )
         };

      } // service
      struct Service
      {
         std::string name;
         serviceframework::optional< std::string> timeout;
         serviceframework::optional< std::vector< std::string>> routes;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            archive & CASUAL_MAKE_NVP( name);
            archive & CASUAL_MAKE_NVP( timeout);
            archive & CASUAL_MAKE_NVP( routes);
         )

         Service& operator += ( const service::Default& rhs);
         friend bool operator == ( const Service& lhs, const Service& rhs);
      };
   } // configuration
} // casual


