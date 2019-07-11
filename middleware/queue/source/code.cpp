//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/code.h"

#include "common/cast.h"

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
                  return "cq";
               }

               std::string message( int code) const override
               {
                  switch( static_cast< queue::code>( code))
                  {
                     case code::ok: return "ok";
                     case code::argument: return "invalid arguments";
                     case code::no_message: return "no message";
                     case code::no_queue: return "no queue";
                     case code::system: return "system";
                  }
                  return "unknown";
               }
            };

            const Category category{};
         } // <unnamed>
      } // local

      std::error_code make_error_code( queue::code code)
      {
         return { common::cast::underlying( code), local::category};
      }

   } // queue
} // casual