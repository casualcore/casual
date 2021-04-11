//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/code/system.h"

#include "common/code/convert.h"
#include "common/code/raise.h"

namespace casual
{
   namespace common::code::system
   {
      namespace last
      {
         std::errc error()
         {
            return static_cast< std::errc>( errno);
         }
         
      } // last

      void raise() noexcept( false)
      {
         code::raise::log( code::convert::to::casual( last::error()));
      }

      void raise( const std::string& context) noexcept( false)
      {
         auto error = last::error();
         code::raise::log( code::convert::to::casual( error), context, " - ", error);
      }
      
   } // common::code::system
} // casual
