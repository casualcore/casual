//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/xatmi/code.h"

#include "common/log/stream.h"

#include <system_error>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace code
      {
         enum class xatmi : int
         {
            ok = 0,
            descriptor = TPEBADDESC,
            no_message = TPEBLOCK,
            argument = TPEINVAL,
            limit = TPELIMIT,
            no_entry = TPENOENT,
            os = TPEOS,
            protocol = TPEPROTO,
            service_error = TPESVCERR,
            service_fail = TPESVCFAIL,
            system = TPESYSTEM,
            timeout = TPETIME,
            transaction = TPETRAN,
            signal = TPGOTSIG,
            buffer_input = TPEITYPE,
            buffer_output = TPEOTYPE,
            event = TPEEVENT,
            service_advertised = TPEMATCH,
         };

         std::error_code make_error_code( xatmi code);

         common::log::Stream& stream( code::xatmi code);

         //! @return a "message" representation of the code.
         const char* message( xatmi code) noexcept;

         //! @return the string representation of the code.
         const char* string( xatmi code) noexcept;


      } // codee
   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::xatmi> : true_type {};
}

//
// To help prevent missuse of "raw codes"
//


#ifndef CASUAL_NO_XATMI_UNDEFINE

#undef TPEBADDESC
#undef TPEBLOCK
#undef TPEINVAL
#undef TPELIMIT
#undef TPENOENT
#undef TPEOS
#undef TPEPROTO
#undef TPESVCERR
#undef TPESVCFAIL
#undef TPESYSTEM
#undef TPETIME
#undef TPETRAN
#undef TPGOTSIG
#undef TPEITYPE
#undef TPEOTYPE
#undef TPEEVENT
#undef TPEMATCH

#endif // CASUAL_NO_XATMI_UNDEFINE

