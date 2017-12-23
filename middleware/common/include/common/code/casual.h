//!
//! casual
//!

#ifndef CASUAL_COMMON_CODE_CASUAL_H_
#define CASUAL_COMMON_CODE_CASUAL_H_

#include "common/log/stream.h"
#include "common/signal.h"

#include <system_error>

namespace casual
{
   namespace common
   {
      namespace code
      {
         enum class casual : int
         {
            //ok = 0,
            shutdown = 1,
            validation,
            invalid_configuration,
            invalid_document,
            invalid_node,
            invalid_version,
         };

         //static_assert( static_cast< int>( signal::ok) == 0, "signal::ok has to be 0");

         std::error_code make_error_code( casual code);

         common::log::Stream& stream( casual code);


      } // code
   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::casual> : true_type {};
}

namespace casual
{
   namespace common
   {
      namespace code
      {
         inline std::ostream& operator << ( std::ostream& out, code::casual value) { return out << std::error_code( value);}
      } // code
   } // common
} // casual

#endif
