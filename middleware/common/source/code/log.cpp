//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/code/log.h"

#include "common/log/category.h"
#include "common/log.h"

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
                     return "log";
                  }

                  std::string message( int code) const override
                  {
                     switch( static_cast< code::log>( code))
                     {
                        case code::log::error:  return "error";
                        case code::log::warning: return "warning";
                        case code::log::information: return "information";
                        case code::log::user: return "user";
                        case code::log::internal: return "internal";
                     }
                     return "<unknown>";
                  }
               };

               const Category category{};
            } // <unnamed>
         } // local

         std::error_condition make_error_condition( code::log code)
         {
            return { cast::underlying( code), local::category};
         }


         std::ostream& stream( const std::error_code& code)
         {
            if( code == log::user || code == log::internal) 
               return common::log::debug;
            if( code == log::warning)
               return common::log::category::warning;
            if( code == log::information)
               return common::log::category::information;

            return common::log::category::error;
         }

      } // code
   } // common
} // casual

