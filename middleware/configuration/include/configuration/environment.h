//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/serialize/macro.h"
#include "common/environment.h"

#include <string>
#include <vector>


namespace casual
{
   namespace configuration
   {
      inline namespace v1 {
      
      namespace environment
      {
         struct Variable
         {
            std::string key;
            std::string value;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               CASUAL_SERIALIZE( key);
               CASUAL_SERIALIZE( value);
            )

            friend bool operator == ( const Variable& lhs, const Variable& rhs);
            friend bool operator < ( const Variable& lhs, const Variable& rhs);
         };

      } // environment

      struct Environment
      {
         std::vector< std::string> files;
         std::vector< environment::Variable> variables;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            CASUAL_SERIALIZE( files);
            CASUAL_SERIALIZE( variables);
         )

         Environment& operator += ( const Environment& value);
      };

      namespace environment
      {

         configuration::Environment get( const std::string& file);

         std::vector< Variable> fetch( configuration::Environment environment);

         std::vector< common::environment::Variable> transform( const std::vector< Variable>& variables);

      } // environment

      } // inline namespace v1
   } // config
} // casual




