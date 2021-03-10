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
               std::system_error handle() noexcept
               {
                  try
                  {
                     throw;
                  }
                  catch( const std::system_error& exception)
                  {
                     return exception;
                  }
                  catch( const std::exception& exception)
                  {
                     common::log::line( log::debug, code::casual::internal_unexpected_value, " exception: ", exception);
                     return std::system_error{ std::error_code{ code::casual::internal_unexpected_value}};
                  }
                  catch( ...)
                  {
                     common::log::line( log::debug, code::casual::internal_unexpected_value, " unknown exception");
                     return std::system_error{ std::error_code{ code::casual::internal_unexpected_value}};
                  }
               }
            } // <unnamed>
         } // local
         

         std::system_error error() noexcept
         {
            return local::handle();
         }


         namespace sink
         {
            void log() noexcept
            {
               const auto error = local::handle();
               common::log::line( code::stream( error.code()), error);
            }

            void error() noexcept
            {
               auto error = local::handle();
               common::log::line( common::log::category::error, error);
            }
            
            void silent() noexcept
            {
               auto error = local::handle();
               common::log::line( common::log::debug, error);
            }
         } // sink

      } // exception
   } // common
} // casual




