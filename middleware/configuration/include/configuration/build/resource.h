//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "serviceframework/namevaluepair.h"
#include <string>

namespace casual
{
   namespace configuration
   {
      namespace build
      {
         struct Resource
         {
            //! key of the resource
            std::string key;

            //! name to correlate configuration
            std::string name;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( key);
               archive & CASUAL_MAKE_NVP( name);
            )
         };
      } // build
   } // configuration
} // casual