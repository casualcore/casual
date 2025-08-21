//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/macro.h"
#include "common/algorithm/container.h"
#include "common/string.h"

#include "casual/argument.h"

#include <string>
#include <vector>

namespace casual
{
   namespace tools::build::setting
   {
      namespace mandatory
      {
         struct Paths
         {
            std::vector< std::string> include;
            std::vector< std::string> library;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( include);
               CASUAL_SERIALIZE( library);
            )
         };

         struct Source
         {
            std::string file;
            bool keep = false;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( file);
               CASUAL_SERIALIZE( keep);
            )
         };

         struct System
         {
            std::string configuration;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( configuration);
            )
         };
         
      } // mandatory

      struct Mandatory
      {

         std::string compiler = "g++";
         std::string output;

         // compile & link directives
         std::vector< std::string> directives;

         std::vector< std::string> libraries;

         mandatory::Paths paths;

         mandatory::Source source;

         mandatory::System system;

         bool verbose = false;
         bool use_defaults = true;


         friend void validate( const Mandatory& settings);

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( compiler);
            CASUAL_SERIALIZE( output);
            CASUAL_SERIALIZE( directives);
            CASUAL_SERIALIZE( libraries);
            CASUAL_SERIALIZE( paths);
            CASUAL_SERIALIZE( source);
            CASUAL_SERIALIZE( system);
            CASUAL_SERIALIZE( verbose);
            CASUAL_SERIALIZE( use_defaults);
         )

         static auto split( std::vector< std::string>& target)
         {
            return [&target]( const std::string& value, const std::vector< std::string>& values)
            {
               auto split_append = [&target]( auto& value)
               {
                  common::algorithm::container::append( common::string::adjacent::split( value, ' '), target);
               };
               split_append( value);
               common::algorithm::for_each( values, split_append);
            };
         }
      };

      namespace mandatory
      {
         std::vector< argument::Option> options( Mandatory& mandatory);
      } // mandatory

      
   } // tools::build::setting
   
} // casual
