//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/flag/xatmi.h"

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
                     no_flags = std::to_underlying( xatmi::Flag::no_flags),
                     no_transaction = std::to_underlying( xatmi::Flag::no_transaction),
                     no_reply = std::to_underlying( xatmi::Flag::no_reply),
                     no_block = std::to_underlying( xatmi::Flag::no_block),
                     no_time = std::to_underlying( xatmi::Flag::no_time),
                     signal_restart = std::to_underlying( xatmi::Flag::signal_restart)
                  };
                  using Flags = common::Flags< async::Flag>;
               } // async

               namespace reply
               {
                  enum class Flag : long
                  {
                     no_flags = std::to_underlying( xatmi::Flag::no_flags),
                     any = std::to_underlying( xatmi::Flag::any),
                     no_change = std::to_underlying( xatmi::Flag::no_change),
                     no_block = std::to_underlying( xatmi::Flag::no_block),
                     no_time = std::to_underlying( xatmi::Flag::no_time),
                     signal_restart = std::to_underlying( xatmi::Flag::signal_restart)
                  };
                  using Flags = common::Flags< reply::Flag>;


               } // reply


               namespace sync
               {
                  enum class Flag : long
                  {
                     no_flags = std::to_underlying( xatmi::Flag::no_flags),
                     no_transaction = std::to_underlying( xatmi::Flag::no_transaction),
                     no_change = std::to_underlying( xatmi::Flag::no_change),
                     no_block = std::to_underlying( xatmi::Flag::no_block),
                     no_time = std::to_underlying( xatmi::Flag::no_time),
                     signal_restart = std::to_underlying( xatmi::Flag::signal_restart)
                  };
                  using Flags = common::Flags< sync::Flag>;
               } // sync

            } // call
         } // service
      } // flag
   } // common
} // casual


