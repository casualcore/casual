//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



namespace casual
{
   namespace queue
   {
      namespace manager
      {
         namespace admin
         {
            namespace service
            {
               namespace name
               {
                  constexpr auto state() { return ".casual/queue/state";}
                  constexpr auto restore() { return ".casual/queue/restore";}
                  constexpr auto list_messages() { return ".casual/queue/list/messages";}
               } // name

            } // service

         } // admin
      } // manager
   } // queue

} // casual


