//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/code.h"

#include "common/cast.h"
#include "common/code/serialize.h"
#include "common/code/category.h"
#include "common/code/log.h"

namespace casual
{

   namespace queue
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
                  switch( static_cast< queue::code>( code))
                  {
                     case code::ok: return "ok";
                     case code::argument: return "invalid-arguments";
                     case code::no_message: return "no-message";
                     case code::no_queue: return "no-queue";
                     case code::system: return "system";
                  }
                  return "unknown";
               }

               // defines the log condition equivalence, so we can compare for logging
               bool equivalent( int code, const std::error_condition& condition) const noexcept override
               {
                  if( ! common::code::is::category< common::code::log>( condition))
                     return false;

                  switch( static_cast< queue::code>( code))
                  {
                     case code::ok:
                     case code::argument:
                     case code::no_message: 
                        return condition == common::code::log::user;

                     case code::no_queue: 
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


      std::error_code make_error_code( queue::code code)
      {
         return { common::cast::underlying( code), local::category};
      }

   } // queue
} // casual