//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/flag/xatmi.h"

namespace casual 
{
   namespace common::flag::service::call
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

         //! indicate that this enum is used as a flag and uses xatmi::Flag as superset (for description)
         consteval xatmi::Flag casual_enum_as_flag_superset( Flag);

         constexpr auto valid_flags = Flag::no_flags | Flag::no_transaction | Flag::no_reply | Flag::no_block | Flag::no_time | Flag::signal_restart;

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

         //! indicate that this enum is used as a flag and uses xatmi::Flag as superset (for description)
         consteval xatmi::Flag casual_enum_as_flag_superset( Flag);

         constexpr auto valid_flags = Flag::no_flags | Flag::any | Flag::no_change | Flag::no_block | Flag::no_time | Flag::signal_restart;

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
         
         //! indicate that this enum is used as a flag and uses xatmi::Flag as superset (for description)
         consteval xatmi::Flag casual_enum_as_flag_superset( Flag);

         constexpr auto valid_flags = Flag::no_flags | Flag::no_transaction | Flag::no_change | Flag::no_block | Flag::no_time | Flag::signal_restart;
      } // sync

   } // common::flag::service::call
} // casual


