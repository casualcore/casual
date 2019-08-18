//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/serialize/archive/type.h"

#include <ostream>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace archive
         {
            std::ostream& operator << ( std::ostream& out, Type value)
            {
               switch( value)
               {
                  case Type::dynamic_type: return out << "dynamic_type";
                  case Type::static_need_named: return out << "static_need_named";
                  case Type::static_order_type: return out << "static_order_type";
               }
               return out << "unknown";
            }

            namespace dynamic
            {
               std::ostream& operator << ( std::ostream& out, Type value)
               {
                  switch( value)
                  {
                     case Type::named: return out << "named";
                     case Type::order_type: return out << "order_type";
                  }
                  return out << "unknown";
               }
            } // dynamic
         } // archive
      } // serialize
   } // common
} // casual