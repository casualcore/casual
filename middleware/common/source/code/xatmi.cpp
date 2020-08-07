//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/code/xatmi.h"
#include "common/code/category.h"
#include "common/code/log.h"
#include "common/code/serialize.h"

#include "common/log/category.h"
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
                     return "xatmi";
                  }

                  std::string message( int code) const override
                  {
                     return code::description( static_cast< code::xatmi>( code));
                  }

                  bool equivalent( int code, const std::error_condition& condition) const noexcept override
                  {
                     if( ! is::category< code::log>( condition))
                        return false;

                     switch( static_cast< code::xatmi>( code))
                     {
                        // user
                        case code::xatmi::signal:
                        case code::xatmi::no_message:
                        case code::xatmi::argument:
                        case code::xatmi::descriptor:
                        case code::xatmi::service_fail:
                        case code::xatmi::no_entry:
                        case code::xatmi::service_advertised:
                        case code::xatmi::timeout:
                        case code::xatmi::transaction:
                        case code::xatmi::buffer_input:
                        case code::xatmi::buffer_output: 
                           return condition == code::log::user;

                        // rest is error
                        default:
                           return condition == code::log::error;
                     }
                  }
               };

               const auto& category = code::serialize::registration< Category>( 0x19f3bafea0c74ca1a4fd884009762ee2_uuid);

            } // <unnamed>
         } // local

         const char* description( code::xatmi code) noexcept
         {
            switch( code)
            {
               case xatmi::ok: return "OK";
               case xatmi::descriptor: return "TPEBADDESC";
               case xatmi::no_message: return "TPEBLOCK";
               case xatmi::argument: return "TPEINVAL";
               case xatmi::limit: return "TPELIMIT";
               case xatmi::no_entry: return "TPENOENT";
               case xatmi::os: return "TPEOS";
               case xatmi::protocol: return "TPEPROTO";
               case xatmi::service_error: return "TPESVCERR";
               case xatmi::service_fail: return "TPESVCFAIL";
               case xatmi::system: return "TPESYSTEM";
               case xatmi::timeout: return "TPETIME";
               case xatmi::transaction: return "TPETRAN";
               case xatmi::signal: return "TPGOTSIG";
               case xatmi::buffer_input: return "TPEITYPE";
               case xatmi::buffer_output: return "TPEOTYPE";
               case xatmi::event: return "TPEEVENT";
               case xatmi::service_advertised: return "TPEMATCH";
            }
            return "unknown";
         }

         
         std::error_code make_error_code( code::xatmi code) noexcept
         {
            return { cast::underlying( code), local::category};
         }


      } // code
   } // common
} // casual
