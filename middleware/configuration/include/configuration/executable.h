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
            common::platform::size::type instances = 1;
            std::vector< std::string> memberships;
            Environment environment;
            bool restart = false;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( instances));
               CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( restart));
               CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( memberships));
               CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( environment));
            )
         };

      } // executable

      struct Executable
      {
         std::string path;
         common::optional< std::string> alias;
         common::optional< std::string> note;

         common::optional< std::vector< std::string>> arguments;

         common::optional< common::platform::size::type> instances;
         common::optional< std::vector< std::string>> memberships;
         common::optional< Environment> environment;
         common::optional< bool> restart;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            
            CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( path));
            CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( alias));
            CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( note));
            CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( arguments));

            CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( instances));
            CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( memberships));
            CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( environment));
            CASUAL_SERIALIZE( CASUAL_NAMED_VALUE( restart));
            
         )

         Executable& operator += ( const executable::Default& value);

      };
   } // configuration
} // casual


