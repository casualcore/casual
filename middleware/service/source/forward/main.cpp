//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/forward/cache.h"

#include "common/exception/handle.h"
#include "common/arguments.h"

namespace casual
{

   namespace service
   {
      namespace forward
      {

         int main( int argc, char **argv)
         {
            try
            {
               {
                  casual::common::Arguments parser{ {}};
                  parser.parse( argc, argv);
               }

               Cache cache;
               cache.start();
            }
            catch( ...)
            {
               return common::exception::handle();
            }
            return 0;
         }
      } // forward
   } // service
} // casual



int main( int argc, char **argv)
{
   return casual::service::forward::main( argc, argv);
}
