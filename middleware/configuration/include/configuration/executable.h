//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "configuration/environment.h"

#include "common/serialize/macro.h"
#include "common/optional.h"

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
         common::optional< std::string> alias;
         common::optional< std::string> note;

         common::optional< std::vector< std::string>> arguments;

         common::optional< platform::size::type> instances;
         common::optional< std::vector< std::string>> memberships;
         common::optional< Environment> environment;
         common::optional< bool> restart;

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


