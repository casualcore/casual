//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/code/xatmi.h"
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
               const char* message( xatmi code)
               {
                  switch( code)
                  {
                     case xatmi::ok: return "";
                     case xatmi::descriptor: return "TPEBADDESC: invalid descriptor was given";
                     case xatmi::no_message: return "TPEBLOCK: no message ready to consume";
                     case xatmi::argument: return "TPEINVAL: invalid arguments was given";
                     case xatmi::limit: return "TPELIMIT: system limit was reached";
                     case xatmi::no_entry: return "TPENOENT: failed to lookup service";
                     case xatmi::os: return "TPEOS: operating system level error detected";
                     case xatmi::protocol: return "TPEPROTO: routine was called in an improper context";
                     case xatmi::service_error: return "TPESVCERR: system level service error";
                     case xatmi::service_fail: return "TPESVCFAIL: application level service error";
                     case xatmi::system: return "TPESYSTEM: system level error detected";
                     case xatmi::timeout: return "TPETIME: timeout reach during execution";
                     case xatmi::transaction: return "TPETRAN: transaction error detected";
                     case xatmi::signal: return "TPGOTSIG: signal was caught during blocking execution";
                     case xatmi::buffer_input: return "TPEITYPE: invalid input buffer type";
                     case xatmi::buffer_output: return "TPEOTYPE: invalid output buffer type";
                     case xatmi::event: return "TPEEVENT: conversation event was received";
                     case xatmi::service_advertised: return "TPEMATCH: service is already advertised";
                     default: return "unknown";
                  }
               }

               struct Category : std::error_category
               {
                  const char* name() const noexcept override
                  {
                     return "xatmi";
                  }

                  std::string message( int code) const override
                  {
                     return local::message( static_cast< code::xatmi>( code));
                  }
               };

               const Category category{};

            } // <unnamed>
         } // local

         std::error_code make_error_code( xatmi code)
         {
            return std::error_code( static_cast< int>( code), local::category);
         }

         common::log::Stream& stream( code::xatmi code)
         {
            switch( code)
            {
               // information
               case code::xatmi::signal:
               case code::xatmi::limit: return common::log::category::information;

               // debug
               case code::xatmi::no_message:
               case code::xatmi::argument:
               case code::xatmi::descriptor:
               case code::xatmi::service_fail:
               case code::xatmi::no_entry:
               case code::xatmi::service_advertised:
               case code::xatmi::timeout:
               case code::xatmi::transaction:
               case code::xatmi::buffer_input:
               case code::xatmi::buffer_output: return common::log::debug;

               // rest is errors
               default: return common::log::category::error;
            }
         }

         const char* message( xatmi code) noexcept
         {
            return local::message( code);
         }

      } // code
   } // common
} // casual
