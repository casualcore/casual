//!
//! casual
//!

#include "common/exception/xatmi.h"
#include "common/exception/handle.h"
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
            namespace local
            {
               namespace
               {
                  template< typename E> 
                  void log( E&& exception)
                  {
                     code::stream( exception.type()) << exception << std::endl;
                  }
               } // <unnamed>
            } // local
            code::xatmi handle()
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
                  local::log( exception);
                  return exception.type();
               }
               catch ( const common::exception::system::invalid::Argument& exception)
               {
                  local::log( exception);
                  return code::xatmi::argument;
               }
               catch( ...)
               {
                  common::exception::handle();
               }

               return code::xatmi::system;
            }
         } // xatmi

      } // exception
	} // common
} // casual



