//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include <system_error>

namespace casual
{
   namespace common
   {
      namespace code
      {
         namespace system
         {
            namespace last
            {
               //! returns the last error code from errno
               std::errc error();
               
            } // last
            
            //! converts last system error to code::casual and raise an exception
            [[noreturn]] void raise() noexcept( false);
            [[noreturn]] void raise( const std::string& context) noexcept( false);

         } // system

      } // code
   } // common
} // casual

