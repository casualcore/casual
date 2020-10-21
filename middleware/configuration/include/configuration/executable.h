//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "configuration/environment.h"

#include "common/serialize/macro.h"
#include <optional>

namespace casual
{
   namespace configuration
   {
      namespace executable
      {
         struct Default
         {
            platform::size::type instances = 1;
            std::vector< std::string> memberships;
            Environment environment;
            bool restart = false;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( instances);
               CASUAL_SERIALIZE( restart);
               CASUAL_SERIALIZE( memberships);
               CASUAL_SERIALIZE( environment);
            )
         };

      } // executable

      struct Executable
      {
         std::string path;
         std::optional< std::string> alias;
         std::optional< std::string> note;

         std::optional< std::vector< std::string>> arguments;

         std::optional< platform::size::type> instances;
         std::optional< std::vector< std::string>> memberships;
         std::optional< Environment> environment;
         std::optional< bool> restart;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            CASUAL_SERIALIZE( path);
            CASUAL_SERIALIZE( alias);
            CASUAL_SERIALIZE( note);
            CASUAL_SERIALIZE( arguments);
            CASUAL_SERIALIZE( instances);
            CASUAL_SERIALIZE( memberships);
            CASUAL_SERIALIZE( environment);
            CASUAL_SERIALIZE( restart);
            
         )

         Executable& operator += ( const executable::Default& value);

      };
   } // configuration
} // casual


