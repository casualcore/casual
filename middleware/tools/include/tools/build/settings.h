//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/serialize/macro.h"
#include "common/algorithm/container.h"
#include "common/string.h"

#include "configuration/model.h"

#include "casual/argument.h"

#include <string>
#include <vector>
#include <filesystem>

namespace casual
{
   namespace tools::build
   {
      namespace settings
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
            std::vector< std::string> globs;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( globs);
            )
         };

         struct Resource
         {
            std::vector< std::string> keys;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( keys);
            )
         };
         
      } // settings

      struct Settings
      {
         std::string compiler = "g++";
         std::string output;

         // compile & link directives
         std::vector< std::string> directives;
         std::vector< std::string> libraries;
         settings::Paths paths;
         settings::Source source;
         settings::System system;

         bool only_generate = false;
         bool verbose = false;
         bool use_defaults = true;
         


         friend void validate( const Settings& settings);

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( compiler);
            CASUAL_SERIALIZE( output);
            CASUAL_SERIALIZE( directives);
            CASUAL_SERIALIZE( libraries);
            CASUAL_SERIALIZE( paths);
            CASUAL_SERIALIZE( source);
            CASUAL_SERIALIZE( system);
            CASUAL_SERIALIZE( only_generate);
            CASUAL_SERIALIZE( verbose);
            CASUAL_SERIALIZE( use_defaults);
         )

      };

      namespace settings
      {
         //! splits values on space and appends to target
         inline auto split( std::vector< std::string>& target)
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


         std::vector< argument::Option> options( Settings& settings);

         namespace resource::key
         {
            argument::Option option( settings::Resource& resource);
         } // resource::key


         //! returns a system model based on the settings. 
         //! If no system paths are provided, environment variable is used to 
         //! try to locate the default system configuration.
         configuration::model::system::Model system( const Settings& settings);

      } // settings

      
   } // tools::build
   
} // casual
