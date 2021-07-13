//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "http/code.h"

#include "common/code/serialize.h"

namespace casual
{
   namespace http
   {

      namespace local
      {
         namespace
         {
            struct Category : std::error_category
            {
               const char* name() const noexcept override
               {
                  return "http";
               }

               std::string message( int code) const override
               {
                  switch( static_cast< http::code>( code))
                  {
                     case code::ok: return "OK";

                     case code::bad_request: return "Bad Request";
                     case code::not_found: return "Not Found";
                     case code::request_timeout: return "Request Timeout";

                     case code::internal_server_error: return "Internal Server Error";

                  }
                  return "<unknown>";
               }


            };

            const auto& category = common::code::serialize::registration< Category>( 0xb07458910e114db388242bacfb94550e_uuid);


         } // <unnamed>
      } // local
      
      std::error_code make_error_code( http::code code)
      {
         return { static_cast< int>( code), local::category};
      }
   } // http
   
} // casual