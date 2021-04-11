//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/strong/id.h"

#include <ostream>

namespace casual
{
   namespace common::strong
   {
      namespace resource
      {
         std::ostream& policy::stream( std::ostream& out, platform::resource::native::type value)
         {
            if( value < 0) return out << "E-" << std::abs( value);
            if( value > 0) return out << "L-" << value;
            return out << "nil";
         }

         platform::resource::native::type policy::generate()
         {
            static platform::resource::native::type value{};
            return ++value;
         }

      } // resource

   } // common::strong
} // casual
