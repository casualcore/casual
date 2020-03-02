//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "tools/build/model.h"

#include "common/algorithm.h"
#include "common/string.h"
#include "common/serialize/macro.h"

#include <string>
#include <vector>

namespace casual
{
   namespace tools
   {
      namespace build
      {
         struct Directive
         {

            std::string compiler = "g++";
            std::string output;

            // compile & link directives
            std::vector< std::string> directives;

            std::vector< std::string> libraries;

            struct 
            {
               std::vector< std::string> include;
               std::vector< std::string> library;
            } paths;

            bool verbose = false;
            bool use_defaults = true;


            friend void validate( const Directive& settings);

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( compiler);
               CASUAL_SERIALIZE( output);
               CASUAL_SERIALIZE( directives);
               CASUAL_SERIALIZE( libraries);
               CASUAL_SERIALIZE( paths.include);
               CASUAL_SERIALIZE( paths.library);
               CASUAL_SERIALIZE( verbose);
               CASUAL_SERIALIZE( use_defaults);
            )

            static auto split( std::vector< std::string>& target)
            {
               return [&target]( const std::string& value, const std::vector< std::string>& values)
               {
                  auto split_append = [&target]( auto& value)
                  {
                     common::algorithm::append( common::string::adjacent::split( value, ' '), target);
                  };
                  split_append( value);
                  common::algorithm::for_each( values, split_append);
               };
            }
         };

         
         void task( const std::string& input, const Directive& directive);

      } // build
   } // tools
} // casual