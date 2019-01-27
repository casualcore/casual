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
      struct Group
      {
         std::string name;
         std::string note;

         serviceframework::optional< std::vector< std::string>> resources;
         serviceframework::optional< std::vector< std::string>> dependencies;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            archive & CASUAL_MAKE_NVP( name);
            archive & CASUAL_MAKE_NVP( note);
            archive & CASUAL_MAKE_NVP( resources);
            archive & CASUAL_MAKE_NVP( dependencies);
         })

         Group& operator += ( const Group& rhs);
         friend bool operator == ( const Group& lhs, const Group& rhs);

      };
   } // configuration
} // casual

