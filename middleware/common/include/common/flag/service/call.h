//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CALL_FLAGS_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CALL_FLAGS_H_


#include "common/flag/xatmi.h"

#include "common/cast.h"

namespace casual 
{
   namespace common 
   {
      namespace flag
      {
         namespace service
         {
            namespace call
            {
               namespace async
               {
                  enum class Flag : long
                  {
                     no_flags = cast::underlying( xatmi::Flag::no_flags),
                     no_transaction = cast::underlying( xatmi::Flag::no_transaction),
                     no_reply = cast::underlying( xatmi::Flag::no_reply),
                     no_block = cast::underlying( xatmi::Flag::no_block),
                     no_time = cast::underlying( xatmi::Flag::no_time),
                     signal_restart = cast::underlying( xatmi::Flag::signal_restart)
                  };
                  using Flags = common::Flags< async::Flag>;
               } // async

               namespace reply
               {
                  enum class Flag : long
                  {
                     no_flags = cast::underlying( xatmi::Flag::no_flags),
                     any = cast::underlying( xatmi::Flag::any),
                     no_change = cast::underlying( xatmi::Flag::no_change),
                     no_block = cast::underlying( xatmi::Flag::no_block),
                     no_time = cast::underlying( xatmi::Flag::no_time),
                     signal_restart = cast::underlying( xatmi::Flag::signal_restart)
                  };
                  using Flags = common::Flags< reply::Flag>;


               } // reply


               namespace sync
               {
                  enum class Flag : long
                  {
                     no_flags = cast::underlying( xatmi::Flag::no_flags),
                     no_transaction = cast::underlying( xatmi::Flag::no_transaction),
                     no_change = cast::underlying( xatmi::Flag::no_change),
                     no_block = cast::underlying( xatmi::Flag::no_block),
                     no_time = cast::underlying( xatmi::Flag::no_time),
                     signal_restart = cast::underlying( xatmi::Flag::signal_restart)
                  };
                  using Flags = common::Flags< sync::Flag>;
               } // sync

            } // call
         } // service
      } // flag
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_SERVICE_CONVERSATION_FLAGS_H_
