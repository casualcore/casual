//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/server/argument.h"

namespace casual
{
   namespace transaction
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
                  constexpr auto state() { return ".casual/transaction/state";}

                  namespace scale
                  {
                     constexpr auto instances() { return ".casual/transaction/scale/instances";}
                  } // update
               } // name
            } // service

            common::server::Arguments services( manager::State& state);

         } // manager
      } // admin
   } // transaction
} // casual

