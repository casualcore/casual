//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "xa/code.h"
#include "common/log/stream.h"

#include <system_error>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace code
      {
         enum class ax : int
         {
            join = TM_JOIN,
            resume = TM_RESUME,
            ok = TM_OK,
            error = TMER_TMERR,
            argument = TMER_INVAL,
            protocol = TMER_PROTO
         };

         static_assert( static_cast< int>( ax::ok) == 0, "ax::ok has to be 0");

         std::error_code make_error_code( ax code);

         common::log::Stream& stream( code::ax code);

         enum class xa : int
         {
            rollback_unspecified = XA_RBROLLBACK,
            rollback_communication = XA_RBCOMMFAIL,
            rollback_deadlock = XA_RBDEADLOCK,
            rollback_other = XA_RBOTHER,
            rollback_protocoll = XA_RBPROTO,
            rollback_timeout = XA_RBTIMEOUT,
            rollback_transient = XA_RBTRANSIENT,
            rollback_integrity = XA_RBINTEGRITY,


            no_migrate= XA_NOMIGRATE,
            heuristic_hazard = XA_HEURHAZ,
            heuristic_commit = XA_HEURCOM,
            heuristic_rollback = XA_HEURRB,
            heuristic_mix = XA_HEURMIX,
            retry = XA_RETRY,
            read_only = XA_RDONLY,
            ok = XA_OK,

            outstanding_async = XAER_ASYNC,
            invalid_xid =  XAER_NOTA,
            argument = XAER_INVAL,
            protocol = XAER_PROTO,
            resource_fail = XAER_RMFAIL,
            resource_error = XAER_RMERR,
            duplicate_xid = XAER_DUPID,
            outside = XAER_OUTSIDE,

         };

         static_assert( static_cast< int>( xa::ok) == 0, "xa::ok has to be 0");

         std::error_code make_error_code( xa code);

         common::log::Stream& stream( code::xa code);

      } // code
   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::code::xa> : true_type {};

   template <>
   struct is_error_code_enum< casual::common::code::ax> : true_type {};
}


//
// To help prevent missuse of "raw codes"
//


#ifndef CASUAL_NO_XATMI_UNDEFINE

#undef TM_JOIN
#undef TM_RESUME
#undef TM_OK
#undef TMER_TMERR
#undef TMER_INVAL
#undef TMER_PROTO

#undef XA_RBBASE
#undef XA_RBROLLBACK
#undef XA_RBCOMMFAIL
#undef XA_RBDEADLOCK
#undef XA_RBINTEGRITY
#undef XA_RBOTHER
#undef XA_RBPROTO
#undef XA_RBTIMEOUT
#undef XA_RBTRANSIENT
#undef XA_RBEND

#undef XA_NOMIGRATE
#undef XA_HEURHAZ
#undef XA_HEURCOM
#undef XA_HEURRB
#undef XA_HEURMIX
#undef XA_RETRY
#undef XA_RDONLY
#undef XA_OK
#undef XAER_ASYNC
#undef XAER_RMERR
#undef XAER_NOTA
#undef XAER_INVAL
#undef XAER_PROTO
#undef XAER_RMFAIL
#undef XAER_DUPID
#undef XAER_OUTSIDE

#endif // CASUAL_NO_XATMI_UNDEFINE


