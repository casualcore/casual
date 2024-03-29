//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

namespace casual
{
   namespace queue::manager::admin::service::name
   {
      constexpr auto state = ".casual/queue/state";
      constexpr auto restore = ".casual/queue/restore";
      constexpr auto clear = ".casual/queue/clear";
      constexpr auto recover = ".casual/queue/recover";


      namespace messages
      {
         constexpr auto list = ".casual/queue/messages/list";
         constexpr auto remove = ".casual/queue/messages/remove";
         
      } // messages

      namespace metric
      {
         constexpr auto reset = ".casual/queue/metric/reset";
      } // metric 

      namespace forward
      {
         namespace scale
         {
            constexpr auto aliases = ".casual/queue/forward/scale/aliases";
         } // scale
         
      } // forward
      
   } // queue::manager::admin::service::name
} // casual


