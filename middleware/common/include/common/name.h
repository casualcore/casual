//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <string_view>

namespace casual
{
   namespace common::name
   {
      namespace hidden
      {
         //! @returns true if the `name` is _hidden_ (starts with `.`)
         inline bool name( std::string_view value)
         {
            return value.starts_with( '.');
         }
      } // hidden
   } // common::name
} // casual
