//!
//! casual
//!

#ifndef CASUAL_COMMON_ERROR_CODE_XATMI_H_
#define CASUAL_COMMON_ERROR_CODE_XATMI_H_

#include "xatmi/code.h"

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

         const char* message( xatmi code) noexcept;


      } // codee
   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::xatmi> : true_type {};
}

namespace casual
{
   namespace common
   {
      namespace code
      {
         inline std::ostream& operator << ( std::ostream& out, code::xatmi value) { return out << std::error_code( value);}
      } // code
   } // common
} // casual


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


#endif
