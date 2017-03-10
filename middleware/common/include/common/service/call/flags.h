//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CALL_FLAGS_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CALL_FLAGS_H_


#include "common/flag.h"

#include "xatmi.h"

namespace casual
{
   namespace common
   {
      namespace service
      {
         namespace call
         {
            namespace async
            {
               enum class Flag : long
               {
                  no_transaction = TPNOTRAN,
                  no_reply = TPNOREPLY,
                  no_block = TPNOBLOCK,
                  no_time = TPNOTIME,
                  signal_restart = TPSIGRSTRT
               };
               using Flags = common::Flags< async::Flag>;
            } // async

            namespace reply
            {
               enum class Flag : long
               {
                  any = TPGETANY,
                  no_change = TPNOCHANGE,
                  no_block = TPNOBLOCK,
                  no_time = TPNOTIME,
                  signal_restart = TPSIGRSTRT
               };
               using Flags = common::Flags< reply::Flag>;


            } // reply


            namespace sync
            {
               enum class Flag : long
               {
                  no_transaction = TPNOTRAN,
                  no_change = TPNOCHANGE,
                  no_block = TPNOBLOCK, // ignored
                  no_time = TPNOTIME,
                  signal_restart = TPSIGRSTRT
               };
               using Flags = common::Flags< sync::Flag>;
            } // sync

         } // call
      } // service
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_FLAGS_H_
