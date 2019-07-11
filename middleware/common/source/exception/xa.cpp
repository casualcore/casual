//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/exception/xa.h"
#include "common/log/category.h"


namespace casual
{
   namespace common
   {

      namespace exception
      {
         namespace local
         {
            namespace
            {
               namespace handle
               {
                  void unexpected() 
                  {
                     try
                     {
                        throw;
                     }
                     catch( const exception::base& exception)
                     {
                        log::line( log::category::error, exception);
                     }
                     catch( const std::exception& exception)
                     {
                        log::line( log::category::error, exception.what());
                     }
                     catch( ...)
                     {
                        log::line( log::category::error, "unexpected exception");
                     }
                  }
                  
               } // handle
            } // <unnamed>
         } // local
         namespace ax
         {
                  
            code::ax handle()
            {
               try
               {
                  throw;
               }
               // ax stuff
               catch( const exception& exception)
               {
                  log::line( code::stream( exception.type()), exception);
                  return exception.type();
               }
               catch( ...)
               {
                  local::handle::unexpected();
               }

               return code::ax::error;
            }
         } // xa

         namespace xa
         {
                  
            code::xa handle()
            {
               try
               {
                  throw;
               }
               // xa stuff
               catch( const exception& exception)
               {
                  log::line( code::stream( exception.type()), exception);
                  return exception.type();
               }
               catch( ...)
               {
                  local::handle::unexpected();
               }

               return code::xa::resource_fail;
            }
         } // xa
      } // exception
   } // common
} // casual



