//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/administration/cli.h"

#include "common/exception/guard.h"

#include <iostream>


namespace casual
{  
   namespace administration
   {
      namespace local
      {
         namespace
         {
            void main( int argc, char** argv)
            {
               administration::CLI cli;
               cli.parser()( argc, argv);
            }
         } // <unnamed>
      } // local
      
   } // administration

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::administration::local::main( argc, argv);
   });
}


