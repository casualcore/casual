//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <string_view>

namespace casual
{
   namespace common::serialize::archive
   {
      enum class Type : short
      {
         static_need_named,
         static_order_type,
         dynamic_type,
      };

      std::string_view description( Type value) noexcept;

      namespace dynamic
      {
         enum class Type : short
         {
            named,
            order_type,
         };

         std::string_view description( Type value) noexcept;
      } // dynamic

   } // common::serialize::archive
} // casual