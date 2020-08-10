//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/forward/cache.h"

#include "common/exception/guard.h"
#include "common/argument.h"

namespace casual
{

   namespace service
   {
      namespace forward
      {
         void main( int argc, char **argv)
         {
            casual::common::argument::Parse{ "service forward"}( argc, argv);
      
            Cache cache;
            cache.start();
         }
      } // forward
   } // service
} // casual



int main( int argc, char **argv)
{
   return casual::common::exception::main::guard( [=]()
   {
      casual::service::forward::main( argc, argv);
   });
}
