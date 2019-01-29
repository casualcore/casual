//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "configuration/environment.h"

#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"

namespace casual
{
   namespace configuration
   {
      namespace executable
      {
         struct Default
         {
            serviceframework::platform::size::type instances = 1;
            std::vector< std::string> memberships;
            Environment environment;
            bool restart = false;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( restart);
               archive & CASUAL_MAKE_NVP( memberships);
               archive & CASUAL_MAKE_NVP( environment);
            )
         };

      } // executable

      struct Executable
      {
         std::string path;
         serviceframework::optional< std::string> alias;
         serviceframework::optional< std::string> note;

         serviceframework::optional< std::vector< std::string>> arguments;

         serviceframework::optional< serviceframework::platform::size::type> instances;
         serviceframework::optional< std::vector< std::string>> memberships;
         serviceframework::optional< Environment> environment;
         serviceframework::optional< bool> restart;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            
            archive & CASUAL_MAKE_NVP( path);
            archive & CASUAL_MAKE_NVP( alias);
            archive & CASUAL_MAKE_NVP( note);
            archive & CASUAL_MAKE_NVP( arguments);

            archive & CASUAL_MAKE_NVP( instances);
            archive & CASUAL_MAKE_NVP( memberships);
            archive & CASUAL_MAKE_NVP( environment);
            archive & CASUAL_MAKE_NVP( restart);
            
         )

         Executable& operator += ( const executable::Default& value);

      };
   } // configuration
} // casual


