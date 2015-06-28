//!
//! main.cpp
//!
//! Created on: Jun 28, 2015
//!     Author: Lazan
//!

#include "broker/forward/cache.h"

#include "common/error.h"

namespace casual
{

   namespace broker
   {
      namespace forward
      {

         int main( int argc, char **argv)
         {
            try
            {
               Cache cache;

               cache.start();
            }
            catch( ...)
            {
               return common::error::handler();
            }
            return 0;
         }
      } // forward
   } // broker
} // casual



int main( int argc, char **argv)
{
   return casual::broker::forward::main( argc, argv);
}
