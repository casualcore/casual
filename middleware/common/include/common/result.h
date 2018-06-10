//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

namespace casual
{
   namespace common
   {
      namespace posix
      {
         //!
         //! checks posix result, and throws appropriate exception if error
         //! 
         //! @returns value of result, if no errors detected 
         //!
         int result( int result);

         namespace log
         {
            //!
            //! checks posix result, and logs if error
            //!
            void result( int result) noexcept;
         } // log
      } // posix
   } // common
} // casual