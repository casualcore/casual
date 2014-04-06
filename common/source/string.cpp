//!
//! string.cpp
//!
//! Created on: Apr 6, 2014
//!     Author: Lazan
//!

#include "common/string.h"
#include <cxxabi.h>


namespace casual
{
   namespace common
   {
      namespace type
      {
         namespace internal
         {
            std::string name( const std::type_info& type)
            {
               int status;
               return abi::__cxa_demangle( type.name(), 0, 0, &status);
            }
         } // internal


      } // type
   } // common
} // casual
