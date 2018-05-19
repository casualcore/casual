//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "buffer/field.h"
#include "buffer/internal/field.h"
#include "buffer/internal/common.h"

#include "common/exception/handle.h"

#include "common/argument.h"

#include "xatmi.h"

#include <iostream>

namespace casual
{

   void main(int argc, char **argv)
   {
      std::string format;

      {
         common::argument::Parse parse{ R"(

(xml|json|yaml|ini) --> casual-fielded-buffer

reads from stdin an assumes a human readable structure in the supplied format
for a casual-fielded-buffer, and transform this to an actual casual-fielded-buffer,
and prints this to stdout.)",
            common::argument::Option( std::tie( format), { "--format"}, "which format to expect on stdin (xml|json|yaml|ini)")
         };
         parse( argc, argv);
      }

      auto buffer = buffer::field::internal::serialize( std::cin, format);

      long used = 0;
      casual_field_explore_buffer( buffer, nullptr, &used);

      common::log::line( buffer::verbose::log, "buffer used: ", used);

      std::cout.write( buffer, used);

      tpfree( buffer);

   }

} // casual



int main(int argc, char **argv)
{
   try
   {
      casual::main( argc, argv);
      return 0;
   }
   catch( ...)
   {
      return casual::common::exception::handle( std::cerr);
   }
}
