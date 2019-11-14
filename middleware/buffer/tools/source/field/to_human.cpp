//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/buffer/internal/field.h"
#include "casual/buffer/internal/common.h"
#include "common/exception/handle.h"

#include "common/argument.h"

#include <iostream>

namespace casual
{
   namespace local
   {
      namespace
      {
         void main(int argc, char **argv)
         {
            std::string format;

            {
               auto complete_format = []( auto values, bool) -> std::vector< std::string>{
                  return { "json", "yaml", "xml", "ini"};
               };

               common::argument::Parse parse{ R"(

casual-fielded-buffer --> human readable

reads from stdin an assumes a casual-fielded-buffer,
and transform this to a human readable structure in the supplied format,
and prints this to stdout.)",
                  common::argument::Option( std::tie( format), complete_format, { "--format"}, "which format to transform to")
               };
               parse( argc, argv);
            }

            auto pipe = []( auto& format)
            {
               buffer::field::internal::payload::stream( 
                  common::buffer::payload::binary::stream( std::cin), 
                  std::cout, format);
            };

            // allways wait for at least one payload
            pipe( format);

            // consume until end of file
            while( std::cin.peek() != std::istream::traits_type::eof())
               pipe( format);

         }
      } // <unnamed>
   } // local
} // casual

int main(int argc, char **argv)
{
   return casual::common::exception::guard( [&]()
   {
      casual::local::main( argc, argv);
   });
}
