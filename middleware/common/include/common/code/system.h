//!
//! casual
//!

#ifndef CASUAL_COMMON_CODE_SYSTEM_H_
#define CASUAL_COMMON_CODE_SYSTEM_H_

#include "common/log/stream.h"

#include <system_error>

namespace casual
{
   namespace common
   {
      namespace code
      {

         using system = std::errc;


         common::log::Stream& stream( system code);

         namespace last
         {
            namespace system
            {
               //!
               //! returns the last error code from errno
               //!
               code::system error();
            } // system

         } // last

      } // code
   } // common
} // casual


namespace std
{
   inline std::ostream& operator << ( std::ostream& out, std::errc value) 
   { 
      const auto code = std::make_error_code( value);
      return out << code  << " - " << code.message();
   }
} // std

#endif
