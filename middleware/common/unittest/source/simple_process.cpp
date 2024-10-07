//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/argument.h"
#include "common/process.h"

#include "common/exception/guard.h"

using namespace casual::common;

int main( int argc, const char** argv)
{
   try
   {
      int return_value = 0;
      casual::argument::parse( "",{ 
         casual::argument::Option( std::tie( return_value), {"-r"}, "bla")
      }, argc, argv);

      process::sleep( std::chrono::milliseconds( 100));

      return return_value;
   }
   catch( ...)
   {
      return exception::capture().code().value();
   }
}
