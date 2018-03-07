//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/arguments.h"
#include "common/process.h"

#include "common/exception/handle.h"

using namespace casual::common;

int main( int argc, char** argv)
{

   try
   {
      int returnValue = 0;
      Arguments args{ { argument::directive( {"-r"}, "bla", returnValue)}};


      args.parse( argc, argv);

      process::sleep( std::chrono::milliseconds( 100));

      return returnValue;
   }
   catch( ...)
   {
      return exception::handle();
   }
}
