//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_GROUP_C_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_GROUP_C_

#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"

namespace casual
{
   namespace configuration
   {
      namespace group
      {

         struct Group
         {
            Group();
            Group( std::function< void(Group&)> foreign);

            std::string name;
            std::string note;

            serviceframework::optional< std::vector< std::string>> resources;
            serviceframework::optional< std::vector< std::string>> dependencies;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( note);
               archive & CASUAL_MAKE_NVP( resources);
               archive & CASUAL_MAKE_NVP( dependencies);
            )

            friend bool operator == ( const Group& lhs, const Group& rhs);
         };
      } // group
   } // configuration
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_GROUP_C_
