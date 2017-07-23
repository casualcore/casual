//!
//! casual
//!

#include "common/exception/xatmi.h"
#include "common/log/category.h"


namespace casual
{
   namespace common
   {

      namespace exception
      {

         int handle()
         {
            try
            {
               throw;
            }

            //
            // xatmi stuff
            //
            catch( const exception::xatmi::exception& exception)
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

            return static_cast< int>( error::code::xatmi::system);
         }

      } // exception
	} // common
} // casual



