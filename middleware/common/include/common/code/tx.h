//!
//! casual
//!

#ifndef CASUAL_COMMON_ERROR_CODE_TX_H_
#define CASUAL_COMMON_ERROR_CODE_TX_H_

#include "tx/code.h"
#include "common/log/stream.h"

#include <system_error>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace code
      {

         enum class tx : int
         {
            not_supported = TX_NOT_SUPPORTED,
            ok = TX_OK,
            outside = TX_OUTSIDE,
            rollback = TX_ROLLBACK,
            mixed = TX_MIXED,
            hazard = TX_HAZARD,
            protocol = TX_PROTOCOL_ERROR,
            error = TX_ERROR,
            fail = TX_FAIL,
            argument = TX_EINVAL,
            committed = TX_COMMITTED,
            no_begin = TX_NO_BEGIN,

            no_begin_rollback = TX_ROLLBACK_NO_BEGIN,
            no_begin_mixed = TX_MIXED_NO_BEGIN,
            no_begin_hazard = TX_HAZARD_NO_BEGIN,
            no_begin_committed = TX_COMMITTED_NO_BEGIN,
         };

         static_assert( static_cast< int>( tx::ok) == 0, "tx::ok has to be 0");

         std::error_code make_error_code( tx code);

         common::log::Stream& stream( code::tx code);


      } // code

   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::tx> : true_type {};
}

namespace casual
{
   namespace common
   {
      namespace code
      {
         inline std::ostream& operator << ( std::ostream& out, code::tx value) { return out << std::error_code( value);}
      } // code
   } // common
} // casual

//
// To help prevent missuse of "raw codes"
//


#ifndef CASUAL_NO_XATMI_UNDEFINE

#undef TX_NOT_SUPPORTED 
#undef TX_OK
#undef TX_OUTSIDE
#undef TX_ROLLBACK
#undef TX_MIXED
#undef TX_HAZARD
#undef TX_PROTOCOL_ERROR
#undef TX_ERROR
#undef TX_FAIL
#undef TX_EINVAL
#undef TX_COMMITTED
#undef TX_NO_BEGIN

#undef TX_ROLLBACK_NO_BEGIN
#undef TX_MIXED_NO_BEGIN
#undef TX_HAZARD_NO_BEGIN
#undef TX_COMMITTED_NO_BEGIN

#endif // CASUAL_NO_XATMI_UNDEFINE


#endif
