//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/transcode.h"

#include <span>
#include <concepts>

namespace casual
{
   namespace common::strong
   {
      template< typename T, typename Tag>
      struct Span : std::span< T>
      {
         using base_type = std::span< T>;
         using base_type::base_type;
      };

      template< concepts::binary::value_type T, typename Tag>
      std::ostream& operator << ( std::ostream& out, Span< T, Tag> span)
      {
         return transcode::hex::encode( out, span);
      }

      
   } // common::strong
} // casual
