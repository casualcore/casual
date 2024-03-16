//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "casual/xatmi/flag.h"
#include "common/flag.h"

namespace casual 
{
   namespace common::flag::xatmi 
   {

      enum class Flag : long
      {
         no_flags = 0,
         no_block = TPNOBLOCK,
         signal_restart = TPSIGRSTRT,
         no_reply = TPNOREPLY,
         no_transaction = TPNOTRAN,
         in_transaction = TPTRAN,
         no_time = TPNOTIME,
         any = TPGETANY,
         no_change = TPNOCHANGE,
         conversation = TPCONV,
         send_only = TPSENDONLY,
         receive_only = TPRECVONLY
      };

      std::string_view description( Flag value) noexcept;

      // indicate that this enum is used as a flag
      consteval void casual_enum_as_flag( Flag);
      

      enum class Event : long
      {
         absent = 0,
         disconnect = TPEV_DISCONIMM,
         send_only = TPEV_SENDONLY,
         service_error = TPEV_SVCERR,
         service_fail = TPEV_SVCFAIL,
         service_success = TPEV_SVCSUCC
      };

      std::string_view description( Event value) noexcept;

      // indicate that this enum is used as a flag
      consteval void casual_enum_as_flag( Event);

      enum class Return : int
      {
         fail = TPFAIL,
         success = TPSUCCESS,
      };

      std::string_view description( Return value) noexcept;

   } // common::flag::xatmi 
} // casual 

//
// To help prevent missuse of "raw flags"
//
#ifndef CASUAL_NO_XATMI_UNDEFINE

#undef TPSUCCESS
#undef TPFAIL

#undef TPNOBLOCK 
#undef TPSIGRSTRT
#undef TPNOREPLY
#undef TPNOTRAN
#undef TPTRAN
#undef TPNOTIME
#undef TPGETANY
#undef TPNOCHANGE
#undef TPCONV
#undef TPSENDONLY
#undef TPRECVONLY

#undef TPEV_DISCONIMM
#undef TPEV_SVCERR
#undef TPEV_SVCFAIL
#undef TPEV_SVCSUCC
#undef TPEV_SENDONLY

#endif //CASUAL_NO_FLAG_UNDEFINE

