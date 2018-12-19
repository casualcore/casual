//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include <iostream>

extern "C"
{
   int example_main( int argc, char** argv)
   {
      
      std::cout << argv[ 0] <<  " - example_main called\n";
      
      return 0;
   } // main
}