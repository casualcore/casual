#include "common/strong/id.h"

#include <ostream>

namespace casual
{
   namespace common
   {
      namespace strong
      {
         namespace resource
         {
            std::ostream& stream::print( std::ostream& out, platform::resource::native::type value)
            {
               if( value < 0) return out << "E-" << std::abs( value);
               if( value > 0) return out << "L-" << value;
               return out << "invalid";
            }
         } // resource
      } // strong
   } // common
} // casual