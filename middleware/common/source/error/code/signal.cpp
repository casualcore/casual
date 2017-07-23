//!
//! casual
//!

#include "common/error/code/signal.h"

#include "common/log.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace error
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
                        switch( static_cast< code::signal>( code))
                        {
                           //case signal::ok: return "ok";

                           default: return "unknown";
                        }
                     }
                  };

                  const Category category = {};

               } // <unnamed>
            } // local

            std::error_code make_error_code( signal code)
            {
               return { static_cast< int>( code), local::category};
            }

            common::log::Stream& stream( code::signal code)
            {
               switch( code)
               {
                  // debug
                  //case casual::ok: return common::log::debug;

                  // rest is errors
                  default: return common::log::category::error;
               }
            }
            
         } // code 
      } // error
   } // common
} // casual