//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_FLAGS_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_FLAGS_H_


#include "common/flag.h"

#include "xatmi.h"

namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace conversation
         {
            enum class Event : long
            {
               absent = 0,
               disconnect = TPEV_DISCONIMM,
               send_only = TPEV_SENDONLY,
               service_error = TPEV_SVCERR,
               service_fail = TPEV_SVCFAIL,
               service_success = TPEV_SVCSUCC,
            };

            using Events = common::Flags< Event>;

            namespace connect
            {
               enum class Flag : long
               {
                  no_transaction = TPNOTRAN,
                  send_only = TPSENDONLY,
                  receive_only = TPRECVONLY,
                  no_block = TPNOBLOCK,
                  no_time = TPNOTIME,
                  signal_restart = TPSIGRSTRT
               };

               using Flags = common::Flags< connect::Flag>;

            } // connect

            namespace send
            {
               enum class Flag : long
               {
                  receive_only = TPRECVONLY,
                  no_block = TPNOBLOCK,
                  no_time = TPNOTIME,
                  signal_restart = TPSIGRSTRT
               };

               using Flags = common::Flags< send::Flag>;
            } // send

            namespace receive
            {
               enum class Flag : long
               {
                  no_change = TPNOCHANGE,
                  no_block = TPNOBLOCK,
                  no_time = TPNOTIME,
                  signal_restart = TPSIGRSTRT
               };

               using Flags = common::Flags< receive::Flag>;

            } // receive

         } // conversation

      } // service
   } // common



} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_FLAGS_H_
