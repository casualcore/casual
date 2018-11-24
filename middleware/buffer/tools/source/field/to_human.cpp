//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "buffer/internal/field.h"
#include "buffer/internal/common.h"
#include "common/exception/handle.h"

#include "common/argument.h"

#include "xatmi.h"


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

            buffer::field::internal::payload::stream( 
               common::buffer::payload::binary::stream( std::cin), 
               std::cout, format);

         }
      } // <unnamed>
   } // local
} // casual



int main(int argc, char **argv)
{
   try
   {
      casual::local::main( argc, argv);
      return 0;
   }
   catch( ...)
   {
      return casual::common::exception::handle( std::cerr);
   }
}
