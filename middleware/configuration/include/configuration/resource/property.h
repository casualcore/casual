//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "serviceframework/namevaluepair.h"
#include "serviceframework/platform.h"

#include <vector>
#include <string>


namespace casual
{
   namespace configuration
   {
      namespace resource
      {

         struct Property
         {
            Property();
            Property( std::function< void(Property&)> foreign);

            std::string key;
            std::string server;
            std::string xa_struct_name;

            std::vector< std::string> libraries;

            struct
            {
               std::vector< std::string> include;
               std::vector< std::string> library;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( include);
                  archive & CASUAL_MAKE_NVP( library);
               )

            } paths;

            serviceframework::optional< std::string> note;


            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( key);
               archive & CASUAL_MAKE_NVP( server);
               archive & CASUAL_MAKE_NVP( xa_struct_name);
               archive & CASUAL_MAKE_NVP( libraries);
               archive & CASUAL_MAKE_NVP( paths);
               archive & CASUAL_MAKE_NVP( note);
            )
         };

         namespace property
         {
            std::vector< resource::Property> get( const std::string& file);

            std::vector< resource::Property> get();

         } // property


         void validate( const std::vector< Property>& properties);

      } // resource
   } // config
} // casual


