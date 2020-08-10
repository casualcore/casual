//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <system_error>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace code
      {
         enum class log : int
         {
            error = 1,
            warning,
            information,

            user,
            internal,
         };

         std::error_condition make_error_condition( code::log code);

         //! @returns the corresponding log stream for the error_code
         //! if the error code category has not implemented equality for this error_condition
         //! log::category::error is returned
         std::ostream& stream( const std::error_code& code);

      } // code
   } // common
} // casual

namespace std
{
   template <>
   struct is_error_condition_enum< casual::common::code::log> : true_type {};
}
