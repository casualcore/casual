//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "tools/build/resource.h"

#include "common/algorithm.h"
#include "common/string.h"

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


            void add( const std::vector< Resource>& resources);

            friend void validate( const Directive& settings);

            auto cli_directives()
            {
               return [&]( const std::vector< std::string>& values)
               {
                  // TODO: v2.0 do not split strings - make it more explicit
                  // and strict.
                  for( auto value : values)
                  {
                     auto splitted = common::string::adjacent::split( value, ' ');
                     common::algorithm::append_unique( splitted, directives);
                  } 
                  
               };
            }

            friend std::ostream& operator << ( std::ostream& out, const Directive& value);
         };
         

         void task( const std::string& input, const Directive& directive);

      } // build
   } // tools
} // casual