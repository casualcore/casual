//!
//! casual
//!

#include "common/exception/xatmi.h"
#include "common/log/category.h"

#include "common/exception/system.h"


namespace casual
{
   namespace common
   {

      namespace exception
      {
         namespace xatmi
         {
            error::code::xatmi handle()
            {
               try
               {
                  throw;
               }

               //
               // xatmi stuff
               //
               catch( const exception& exception)
               {
                  error::code::stream( exception.type()) << exception << std::endl;
                  return exception.type();
               }
               catch ( const common::exception::system::invalid::Argument& exception)
               {
                  error::code::stream( exception.type()) << exception << std::endl;
                  return error::code::xatmi::argument;
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

               return error::code::xatmi::system;
            }
         } // xatmi

      } // exception
	} // common
} // casual



