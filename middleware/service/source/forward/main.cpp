//!
//! casual
//!

#include "service/forward/cache.h"

#include "common/error.h"
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
   } // service
} // casual



int main( int argc, char **argv)
{
   casual::common::Arguments parser{ {}};

   try
   {
      parser.parse( argc, argv);
      return casual::service::forward::main( argc, argv);
   }
   catch( const std::exception& exception)
   {
      std::cerr << "error: " << exception.what() << std::endl;
   }
   return 10;

}
