//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/argument.h"
#include "common/process.h"

#include "common/exception/guard.h"

using namespace casual::common;

int main( int argc, char** argv)
{
   try
   {
      int return_value = 0;
      argument::Parse{ "", 
         argument::Option( std::tie( return_value), {"-r"}, "bla")
      }( argc, argv);

      process::sleep( std::chrono::milliseconds( 100));

      return return_value;
   }
   catch( ...)
   {
      return exception::error().code().value();
   }
}
