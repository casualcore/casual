//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_GROUP_C_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIGURATION_GROUP_C_

#include "sf/namevaluepair.h"
#include "sf/platform.h"

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

            sf::optional< std::vector< std::string>> resources;
            sf::optional< std::vector< std::string>> dependencies;

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
