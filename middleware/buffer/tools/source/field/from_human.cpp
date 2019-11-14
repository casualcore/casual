//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/buffer/field.h"
#include "casual/buffer/internal/field.h"
#include "casual/buffer/internal/common.h"

#include "common/exception/handle.h"

#include "common/argument.h"

#include <iostream>

namespace casual
{

   void main(int argc, char **argv)
   {
      std::string format;

      {
         auto complete_format = []( auto values, bool) -> std::vector< std::string>
         {
            return { "json", "yaml", "xml", "ini"};
         };

         common::argument::Parse{ R"(

human readable --> casual-fielded-buffer

reads from stdin and assumes a human readable structure in the supplied format
for a casual-fielded-buffer, and transform this to an actual casual-fielded-buffer,
and prints this to stdout.)",
            common::argument::Option( std::tie( format), complete_format, { "--format"}, "which format to expect on stdin"),
         }( argc, argv);
      }

      common::buffer::payload::binary::stream( 
         buffer::field::internal::payload::stream( std::cin, format), 
         std::cout);
   }

} // casual



int main(int argc, char **argv)
{
   return casual::common::exception::guard( [&]()
   {
      casual::main( argc, argv);
   });
}
