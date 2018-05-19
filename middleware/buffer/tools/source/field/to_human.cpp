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

   std::vector< char> read( std::istream& in)
   {
      std::vector< char> result;
      constexpr auto batch = 1024;
      while( in)
      {
         auto offset = result.size();
         result.resize( offset + batch);
         in.read( result.data() + offset, batch);
      }
      result.resize( result.size() - ( batch - in.gcount()));

      return result;
   }

   void main(int argc, char **argv)
   {
      std::string format;

      {
         common::argument::Parse parse{ R"(

casual-fielded-buffer --> (xml|json|yaml|ini)

reads from stdin an assumes a casual-fielded-buffer,
and transform this to a human readable structure in the supplied format,
and prints this to stdout.)",
            common::argument::Option( std::tie( format), { "--format"}, "which format to transform to (xml|json|yaml|ini)")
         };
         parse( argc, argv);
      }

      auto binary = read( std::cin);

      common::log::line( buffer::verbose::log, "binary size: ", binary.size());

      auto buffer = buffer::field::internal::add( std::move( binary));

      assert( buffer);

      buffer::field::internal::serialize( buffer, std::cout, format);

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
