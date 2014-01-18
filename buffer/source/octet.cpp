//!
//! octet.cpp
//!
//! Created on: Dec 26, 2013
//!     Author: Lazan
//!

#include "buffer/octet.h"

#include "common/buffer_context.h"

namespace casual
{
   namespace buffer
   {
      namespace implementation
      {

         class Octet : public common::buffer::implementation::Base
         {
            //
            // We rely on default implementation
            //

            static const bool initialized;
         };




         const bool Octet::initialized = common::buffer::implementation::registrate< Octet>( {{ CASUAL_OCTET, ""}});



      } // implementation
   } // buffer
} // casual
