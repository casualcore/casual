//!
//! casual
//!

#include "common/exception/xa.h"
#include "common/log/category.h"


namespace casual
{
   namespace common
   {

      namespace exception
      {

         namespace xa
         {
                  
            int handle()
            {
               try
               {
                  throw;
               }

               //
               // xa stuff
               //
               catch( const exception& exception)
               {
                  error::code::stream( exception.type()) << exception << std::endl;
                  return exception.code().value();
               }

               catch( const std::system_error& exception)
               {
                  log::category::error << exception << std::endl;
               }
               catch( const std::exception& exception)
               {
                  log::category::error << exception << std::endl;
               }
               catch( ...)
               {
                  log::category::error << " - unexpected exception" << std::endl;
               }

               return static_cast< int>( error::code::xa::resource_fail);
            }
         } // xa
      } // exception
	} // common
} // casual



