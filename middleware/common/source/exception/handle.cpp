//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "common/exception/handle.h"

#include "common/code/log.h"
#include "common/code/casual.h"

#include "common/log/category.h"
#include "common/log.h"


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
              
               std::error_code handle() noexcept
               {
                  try
                  {
                     throw;
                  }
                  catch( std::error_code code)
                  {
                     common::log::line( log::debug, "code: ", code);
                     return code;
                  }
                  catch( const std::system_error& exception)
                  {
                     common::log::line( log::debug, "exception: ", exception);
                     return exception.code();
                  }
                  catch( const std::exception& exception)
                  {
                     common::log::line( log::debug, code::casual::internal_unexpected_value, " exception: ", exception);
                     return std::error_code{ code::casual::internal_unexpected_value};
                  }
                  catch( ...)
                  {
                     common::log::line( log::debug, code::casual::internal_unexpected_value, " unknown exception");
                     return std::error_code{ code::casual::internal_unexpected_value};
                  }
               }
            } // <unnamed>
         } // local
         

         std::error_code code() noexcept
         {
            return local::handle();
         }


         namespace sink
         {
            void log() noexcept
            {
               const auto code = local::handle();
               common::log::line( code::stream( code), code);
            }

            void error() noexcept
            {
               common::log::line( common::log::category::error, local::handle());
            }
            
            void silent() noexcept
            {
               common::log::line( common::log::debug, local::handle());
            }
         } // sink

      } // exception
   } // common
} // casual




