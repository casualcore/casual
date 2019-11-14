//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/macro.h"
#include "casual/platform.h"
#include "common/optional.h"

namespace casual
{
   namespace configuration
   {
      struct Group
      {
         std::string name;
         std::string note;

         common::optional< std::vector< std::string>> resources;
         common::optional< std::vector< std::string>> dependencies;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            CASUAL_SERIALIZE( name);
            CASUAL_SERIALIZE( note);
            CASUAL_SERIALIZE( resources);
            CASUAL_SERIALIZE( dependencies);
         })

         friend bool operator == ( const Group& lhs, const Group& rhs);
         friend bool operator < ( const Group& lhs, const Group& rhs);

      };
   } // configuration
} // casual

