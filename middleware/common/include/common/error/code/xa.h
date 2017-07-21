//!
//! casual
//!

#ifndef CASUAL_COMMON_ERROR_CODE_XA_H_
#define CASUAL_COMMON_ERROR_CODE_XA_H_

#include "xa/code.h"
#include "common/log/stream.h"

#include <system_error>
#include <iosfwd>

namespace casual
{
   namespace common
   {
      namespace error
      {
         namespace code 
         {            
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
      } // error
   } // common
} // casual

namespace std
{
   template <>
   struct is_error_code_enum< casual::common::error::code::xa> : true_type {};
}

namespace casual
{
   namespace common
   {
      namespace error
      {
         namespace code 
         {
            inline std::ostream& operator << ( std::ostream& out, code::xa value) { return out << std::error_code( value);}
         } // code 
      } // error
   } // common
} // casual

#endif