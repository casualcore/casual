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
      namespace log
      {
         class Stream;
      } // log

      namespace code
      {

         using system = std::errc;


         common::log::Stream& stream( system code);

         namespace last
         {
            namespace system
            {
               //!
               //! returns the last error code from errno
               //!
               code::system error();
            } // system

         } // last

      } // code
   } // common
} // casual

