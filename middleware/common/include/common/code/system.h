//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include <system_error>

namespace casual
{
   namespace common::code::system
   {
      namespace last
      {
         //! returns the last error code from errno
         std::errc error();
      } // last

      [[noreturn]] void raise( std::errc code, std::string_view context) noexcept( false);
      
      //! converts last system error to code::casual and raise an exception
      [[noreturn]] void raise() noexcept( false);
      [[noreturn]] inline void raise( std::string_view context) noexcept( false) { raise( last::error(), context);}

   } // common::code::system
} // casual

