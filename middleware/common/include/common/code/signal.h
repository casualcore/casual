//!
//! casual
//!

#ifndef CASUAL_COMMON_CODE_SIGNAL_H_
#define CASUAL_COMMON_CODE_SIGNAL_H_

#include "common/log/stream.h"
#include "common/signal.h"

#include <system_error>

namespace casual
{
   namespace common
   {
      namespace code
      {
         enum class signal : int
         {
            //ok = 0,
            alarm = cast::underlying( common::signal::Type::alarm),
            interupt = cast::underlying( common::signal::Type::interrupt),
            kill = cast::underlying( common::signal::Type::kill),
            quit = cast::underlying( common::signal::Type::quit),
            child = cast::underlying( common::signal::Type::child),
            terminate = cast::underlying( common::signal::Type::terminate),
            user = cast::underlying( common::signal::Type::user),
            pipe = cast::underlying( common::signal::Type::pipe),
         };

         //static_assert( static_cast< int>( signal::ok) == 0, "signal::ok has to be 0");

         std::error_code make_error_code( signal code);

         common::log::Stream& stream( signal code);


      } // code
   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::signal> : true_type {};
}

namespace casual
{
   namespace common
   {
      namespace code
      {
         inline std::ostream& operator << ( std::ostream& out, code::signal value) { return out << std::error_code( value);}
      } // code
   } // common
} // casual

#endif
