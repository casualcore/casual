//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/value/optional.h"
#include "common/code/system.h"
#include "common/signal.h"

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

         
         int result( int result, signal::Set mask);

         namespace log
         {
            //!
            //! checks posix result, and logs if error
            //!
            void result( int result) noexcept;
         } // log

         constexpr auto no_error = static_cast< code::system>( 0);

         using optional_error = value::Optional< code::system, no_error>;

         optional_error error( int result) noexcept;

      } // posix
   } // common
} // casual