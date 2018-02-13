//!
//! casual
//!

#include "common/exception/common.h"

#include <iostream>

namespace casual
{
   namespace common
   {
      namespace exception
      {
         std::ostream& operator << ( std::ostream& out, const base& value)   
         {
            return out << value.code() << ' ' << value.what();
         }
      } // exception
   } // common
} // casual