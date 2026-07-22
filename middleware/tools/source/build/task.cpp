//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!
#include "tools/build/task.h"

#include "tools/common.h"

#include "common/environment/expand.h"
#include "common/process.h"
#include "common/log/line.h"

#include "common/code/raise.h"
#include "common/code/casual.h"

#include <iostream>

namespace casual
{
   using namespace common;
   namespace tools::build
   {
 
      namespace local
      {
         namespace
         {
            
            namespace directive
            {
               constexpr auto output = "-o";
               constexpr auto link = "-l";

               namespace path
               {
                  constexpr auto include = "-I";
                  constexpr auto library = "-L";
               } // path
            } // directive

         } // <unnamed>
      } // local

      void task( const std::filesystem::path& input, const setting::Mandatory& directive)
      {
         verbose::log( directive, "start build task on: ", input);

         validate( directive);

         log::debug( "input: ", input);
         log::debug( "directive: ", directive);

         verbose::log( directive, "directive: ", directive);

         auto arguments = [&]() ->  std::vector< std::string>
         {
            if( directive.use_defaults)
                  return { input, local::directive::output, directive.output};
            else
               return { input};
         }();

         if( directive.use_defaults) 
         {
            algorithm::transform( directive.libraries, arguments, []( auto& value)
            {
               return local::directive::link + value;
            });
         }

         algorithm::container::append( directive.directives, arguments);

         if( directive.use_defaults) 
         {  
            algorithm::transform( directive.paths.include, arguments, []( auto& value)
            {
               return local::directive::path::include + value;
            });

            algorithm::transform( directive.paths.library, arguments, []( auto& value)
            {
               return local::directive::path::library + value;
            });
         }

         // Make sure we resolve environment stuff
         for( auto& argument : arguments)
            argument = common::environment::expand( std::move( argument));

         verbose::log( directive, directive.compiler, " ", common::string::join( arguments, " "));

         {            
            auto capture = common::process::execute( directive.compiler, arguments);
            if( ! capture)
               code::raise::error( code::casual::invalid_argument, "failed to compile - capture: ", capture);
         }
      }

   } // tools::build
} // casual
