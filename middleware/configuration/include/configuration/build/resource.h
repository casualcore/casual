//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/serialize/macro.h"
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

            std::string note;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( key);
               CASUAL_SERIALIZE( name);
               CASUAL_SERIALIZE( note);
            )
         };
      } // build
   } // configuration
} // casual