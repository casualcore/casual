//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/serialize/archive/type.h"

namespace casual
{
   namespace common::serialize::archive
   {
      std::string_view description( Type value) noexcept
      {
         switch( value)
         {
            case Type::dynamic_type: return "dynamic_type";
            case Type::static_need_named: return "static_need_named";
            case Type::static_order_type: return "static_order_type";
         }
         return "unknown";
      }

      namespace dynamic
      {
         std::string_view description( Type value) noexcept
         {
            switch( value)
            {
               case Type::named: return "named";
               case Type::order_type: return "order_type";
            }
            return "unknown";
         }
      } // dynamic
   } // common::serialize::archive
} // casual
