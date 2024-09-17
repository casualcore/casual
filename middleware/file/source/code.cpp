//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "file/code.h"

#include "common/string.h"
#include "common/code/log.h"
#include "common/code/category.h"

namespace casual
{
   namespace file
   {
      namespace local
      {
         namespace
         {
            struct category : std::error_category
            {
               const char* name() const noexcept override
               {
                  return "file";
               }

               std::string message( const int value) const override
               {
                  return std::string{ description( static_cast< code>( value))};
               }

               // defines the log condition equivalence, so we can compare for logging
               bool equivalent( const int value, const std::error_condition& condition) const noexcept override
               {
                  if( ! common::code::is::category< common::code::log>( condition))
                     return false;

                  switch( static_cast< code>( value))
                  {
                     case code::ok:
                     case code::error:
                        return condition == common::code::log::user;
                     default: 
                        return condition == common::code::log::error;
                  }
               }
            };

            const auto& registration = common::code::serialize::registration< category>( 0x3779d91712cf4669a5f3b2dbde4ec66a_uuid);
         } //
      } // local

      std::string_view description( const code value) noexcept
      {
         switch( value)
         {
            case code::ok: return "ok";
            case code::busy: return "busy";
            case code::error: return "error";
         }
         return "<unknown>";
      }

      std::error_code make_error_code( const code value)
      {
         return { std::to_underlying( value), local::registration};
      }

   } // file
} // casual