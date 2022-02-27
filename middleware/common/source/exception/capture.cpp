//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/exception/capture.h"

#include "common/code/casual.h"
#include "common/code/log.h"
#include "common/log.h"

namespace casual
{
   namespace common::exception
   {
      std::system_error capture() noexcept
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
            return std::system_error{ std::error_code{ code::casual::internal_unexpected_value}, exception.what()};
         }
         catch( ...)
         {
            return std::system_error{ std::error_code{ code::casual::internal_unexpected_value}, "unknown exception"};
         }
      }

      void sink() noexcept
      {
         const auto error = capture();
         log::line( code::stream( error.code()), error);
      }

   } // common::exception
} // casual
