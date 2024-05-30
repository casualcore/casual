//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/code/queue.h"

#include "common/string.h"
#include "common/code/log.h"
#include "common/code/category.h"

namespace casual
{
   namespace common::code
   {
      namespace local
      {
         namespace
         {
            struct Category : std::error_category
            {
               const char* name() const noexcept override
               {
                  return "queue";
               }

               std::string message( int code) const override
               {
                  return std::string{ description( static_cast< code::queue>( code))};
               }

               // defines the log condition equivalence, so we can compare for logging
               bool equivalent( int code, const std::error_condition& condition) const noexcept override
               {
                  if( ! common::code::is::category< common::code::log>( condition))
                     return false;

                  switch( static_cast< code::queue>( code))
                  {
                     case code::queue::ok:
                     case code::queue::argument:
                     case code::queue::no_message: 
                        return condition == common::code::log::user;

                     case code::queue::no_queue: 
                        return condition == common::code::log::warning;

                     // rest is error
                     default: 
                        return condition == common::code::log::error;
                  }
               }
            };

            const auto& category = common::code::serialize::registration< Category>( 0xc6518f7c3e244cbcbd877538061c6415_uuid);
         } // <unnamed>
      } // local

      std::string_view description( code::queue value) noexcept
      {
         switch( value)
         {
            case code::queue::ok: return "ok";
            case code::queue::no_message: return "no_message";
            case code::queue::no_queue: return "no_queue";
            case code::queue::argument: return "argument";
            case code::queue::system: return "system";
         }
         return "<unknown>";
      }


      std::error_code make_error_code( code::queue code)
      {
         return { std::to_underlying( code), local::category};
      }

   } // queue
} // casual