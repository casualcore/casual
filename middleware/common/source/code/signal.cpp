//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/code/signal.h"
#include "common/code/category.h"
#include "common/code/log.h"
#include "common/code/serialize.h"

#include "common/log.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace code
      {
         namespace local
         {
            namespace
            {
               struct Category : std::error_category
               {
                  const char* name() const noexcept override
                  {
                     return "signal";
                  }

                  std::string message( int code) const override
                  {
                     return code::description( static_cast< code::signal>( code));
                  }

                  // defines the log condition equivalence, so we can compare for logging
                  bool equivalent( int code, const std::error_condition& condition) const noexcept override
                  {
                     if( ! is::category< code::log>( condition))
                        return false;

                     // every signal is internal stuff
                     return condition == code::log::internal;
                  }
               };

               const auto& category = code::serialize::registration< Category>( 0xa48c8c9013e84903a04cba4e69c22dbd_uuid);

            } // <unnamed>
         } // local

         const char* description( code::signal code)
         {
            switch( code)
            {
               case signal::absent : return "absent";
               case signal::alarm: return "alarm";
               case signal::child: return "child";
               case signal::interrupt: return "interupt";
               case signal::kill: return "kill";
               case signal::pipe: return "pipe";
               case signal::quit: return "quit";
               case signal::terminate: return "terminate";
               case signal::user: return "user";
               case signal::hangup: return "hangup";
            }
            return "unknown";
         }


         std::error_code make_error_code( code::signal code)
         {
            return { static_cast< int>( code), local::category};
         }

         std::ostream& stream( code::signal code)
         {
            switch( code)
            {
               default: return common::log::debug;
            }
         }
      } // code
   } // common
} // casual
