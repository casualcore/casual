//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "gateway/manager/admin/vo.h"

#include "common/server/argument.h"

namespace casual
{
   namespace gateway
   {

      namespace manager
      {
         struct State;

         namespace admin
         {
            namespace service
            {
               namespace name
               {
                  constexpr auto state() { return ".casual/gateway/state";}
               } // name
            } // service

            common::server::Arguments services( manager::State& state);

         } // admin
      } // broker
   } // queue


} // casual


