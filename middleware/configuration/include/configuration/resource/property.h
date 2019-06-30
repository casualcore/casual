//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/macro.h"
#include "common/platform.h"
#include "common/optional.h"

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
                  CASUAL_SERIALIZE( include);
                  CASUAL_SERIALIZE( library);
               )

            } paths;

            common::optional< std::string> note;


            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( key);
               CASUAL_SERIALIZE( server);
               CASUAL_SERIALIZE( xa_struct_name);
               CASUAL_SERIALIZE( libraries);
               CASUAL_SERIALIZE( paths);
               CASUAL_SERIALIZE( note);
            )
         };

         namespace property
         {
            std::vector< resource::Property> get( const std::string& file);

            std::vector< resource::Property> get();

         } // property


         void validate( const std::vector< Property>& properties);

      } // resource
   } // configuration
} // casual


