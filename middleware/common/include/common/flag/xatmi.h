//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#ifndef CASUAL_COMMON_FLAG_XATMI_
#define CASUAL_COMMON_FLAG_XATMI_

#include "xatmi/flag.h"

#include "common/flag.h"

namespace casual 
{
   namespace common 
   {
      namespace flag
      {
         namespace xatmi
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
            using Flags = common::Flags< xatmi::Flag>;

            enum class Event : long
            {
               absent = 0,
               disconnect = TPEV_DISCONIMM,
               send_only = TPEV_SENDONLY,
               service_error = TPEV_SVCERR,
               service_fail = TPEV_SVCFAIL,
               service_success = TPEV_SVCSUCC
            };

            enum class Return : int
            {
               fail = TPFAIL,
               success = TPSUCCESS,
            };
         } // xatmi 
      } // flag
   } // common 
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

#endif
