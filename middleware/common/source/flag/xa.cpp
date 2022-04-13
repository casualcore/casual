//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/flag/xa.h"

namespace casual
{
   namespace common::flag::xa 
   {
      namespace resource 
      {
         std::string_view description( Flag value)
         {
            switch( value)
            {
               case Flag::no_flags: return "no_flags";
               case Flag::dynamic: return "dynamic";
               case Flag::no_migrate: return "no_migrate";
               case Flag::asynchronous: return "asynchronous";
            }
            return "<unknown>";
         }
      } // resource 

      std::string_view description( Flag value)
      {
         switch( value)
         {
            case Flag::no_flags: return "no_flags";
            case Flag::asynchronous: return "asynchronous";
            case Flag::one_phase: return "one_phase";
            case Flag::fail: return "fail";
            case Flag::no_wait: return "no_wait";
            case Flag::resume: return "resume";
            case Flag::success: return "success";
            case Flag::suspend: return "suspend";
            case Flag::start_scan: return "start_scan";
            case Flag::end_scan: return "end_scan";
            case Flag::wait_any: return "wait_any";
            case Flag::join: return "join";
            case Flag::migrate: return "migrate";
            
         }
         return "<unknown>";
      }


      using Flags = common::Flags< xa::Flag>;

   } // common::flag::xa 
   
} // casual