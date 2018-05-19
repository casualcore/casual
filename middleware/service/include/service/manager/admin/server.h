//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once




#include <vector>
#include <string>



#include "common/server/argument.h"

namespace casual
{
   namespace service
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
                  constexpr auto state() { return ".casual/service/state";}

                  namespace metric
                  {
                     constexpr auto reset() { return ".casual/service/metric/reset";}
                  } // metric
               } // name
            } // service

            common::server::Arguments services( manager::State& state);

         } // admin

      } // manager

   } // service
} // casual

