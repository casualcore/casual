//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/serialize/archive/property.h"

namespace casual
{
   namespace common::serialize::archive
   {
      std::string_view description( Property value) noexcept
      {
         switch( value)
         {
            case Property::named: return "named";
            case Property::order: return "order";
            case Property::network: return "network";
            case Property::no_consume: return "no_consume";
         }
         return "unknown";
      }
   } // common::serialize::archive
} // casual
