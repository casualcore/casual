//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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
                     log::line( code::stream( exception.type()), exception);
                  }
               } // <unnamed>
            } // local
            code::xatmi handle()
            {
               try
               {
                  throw;
               }
               // xatmi stuff
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



