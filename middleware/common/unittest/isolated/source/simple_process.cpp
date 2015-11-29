//!
//! simple_process.cpp
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!


#include "common/arguments.h"
#include "common/process.h"


using namespace casual::common;

int main( int argc, char** argv)
{

   int returnValue = 0;
   Arguments args{ { argument::directive( {"-r"}, "bla", returnValue)}};


   args.parse( argc, argv);

   process::sleep( std::chrono::milliseconds( 100));

   return returnValue;
}
