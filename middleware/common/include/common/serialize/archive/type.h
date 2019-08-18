//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace archive
         {
            enum class Type : short
            {
               static_need_named,
               static_order_type,
               dynamic_type,
            }; 
            std::ostream& operator << ( std::ostream& out, Type value);

            namespace dynamic
            {
               enum class Type : short
               {
                  named,
                  order_type,
               }; 
               std::ostream& operator << ( std::ostream& out, Type value);
            } // dynamic
         } // archive
      } // serialize
   } // common
} // casual