//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/flag/xatmi.h"

namespace casual 
{
   namespace common::flag::xatmi 
   {
      std::string_view description( Flag value) noexcept
      {
         switch( value)
         {
            case Flag::no_flags: return "no_flags";
            case Flag::no_block: return "no_block";
            case Flag::signal_restart: return "signal_restart";
            case Flag::no_reply: return "no_reply";
            case Flag::no_transaction: return "no_transaction";
            case Flag::in_transaction: return "in_transaction";
            case Flag::no_time: return "no_time";
            case Flag::any: return "any";
            case Flag::no_change: return "no_change";
            case Flag::conversation: return "conversation";
            case Flag::send_only: return "send_only";
            case Flag::receive_only: return "receive_only";
         }
         return "<unknown>";
      }

      std::string_view description( Event value) noexcept
      {
         switch( value)
         {
            case Event::absent: return "absent";
            case Event::disconnect: return "disconnect";
            case Event::send_only: return "send_only";
            case Event::service_error: return "service_error";
            case Event::service_fail: return "service_fail";
            case Event::service_success: return "service_success";
         }
         return "<unknown>";
      }

      std::string_view description( Return value) noexcept
      {
         switch( value)
         {
            case Return::fail: return "fail";
            case Return::success: return "success";
         }
         return "<unknown>";
      }
   } // common::flag::xatmi 
} // casual 