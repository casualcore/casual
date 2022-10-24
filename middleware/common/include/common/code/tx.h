//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "casual/tx/code.h"

#include <system_error>
#include <iosfwd>

namespace casual
{
   namespace common::code
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
      std::string_view description( code::tx code) noexcept;


      //! "adds" two tx codes, and return the most severe, which could be
      //! the aggregated `no_begin_*` set. 
      tx operator + ( tx lhs, tx rhs) noexcept;
      tx& operator += ( tx& lhs, tx rhs) noexcept;

      static_assert( static_cast< int>( tx::ok) == 0, "tx::ok has to be 0");

      std::error_code make_error_code( code::tx code);
      
   } // common::code
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::tx> : true_type {};
}


// To help prevent missuse of "raw codes"

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


