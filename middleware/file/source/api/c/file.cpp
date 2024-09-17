//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "file/api/c/file.h"
#include "file/api/file.h"
#include "file/code.h"

#include "common/code/casual.h"
#include "common/code/signal.h"
#include "common/code/category.h"
#include "common/exception/capture.h"

#include <utility>

#include <cstring>

namespace casual
{
   namespace file
   {
      namespace local
      {
         namespace
         {
            namespace global
            {
               thread_local std::error_code code;
            } // global
         } //
      } // local


      extern "C" 
      {
         int casual_file_last_error_code()
         {
            if( common::code::is::category< code>( local::global::code))
            {
               switch( code{ local::global::code.value()})
               {
               case code::ok:
                  return CASUAL_FE_OK;
               case code::busy:
                  return CASUAL_FE_BUSY;
               case code::error:
                  return CASUAL_FE_ERROR;
               }
            }

            if( common::code::is::category< common::code::signal>( local::global::code))
            {
               return CASUAL_FE_SIGNAL;
            }

            return CASUAL_FE_ERROR;
         }

         const char* casual_file_last_error_text()
         {
            if( common::code::is::category< code>( local::global::code))
            {
               return description( code{ local::global::code.value()}).data();
            }

            if( common::code::is::category< common::code::signal>( local::global::code))
            {
               return common::code::description( common::code::signal{ local::global::code.value()}).data();
            }

            if( common::code::is::category< common::code::casual>( local::global::code))
            {
               return common::code::description( common::code::casual{ local::global::code.value()}).data();
            }

            return common::code::description( common::code::casual::internal_unexpected_value).data();
         }

         char* casual_file_blocking_reserve( const char* const path)
         {
            local::global::code = code::ok;

            try
            {
               return ::strdup( blocking::reserve( path).native().data());
            }
            catch( ...)
            {
               local::global::code = common::exception::capture().code();
            }

            return nullptr;
         }

         char* casual_file_non_blocking_reserve( const char* const path)
         {
            local::global::code = code::ok;

            try
            {
               if( auto result = non::blocking::reserve( path))
               {
                  return ::strdup( result->native().data());
               }
               else
               {
                  local::global::code = code::busy;
               }
            }
            catch( ...)
            {
               local::global::code = common::exception::capture().code();
            }

            return nullptr;
         }


         void casual_file_release( char* const path)
         {
            ::free( path);
         }

      } // extern C

   } // file
} // casual
