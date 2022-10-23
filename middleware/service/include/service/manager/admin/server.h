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
   namespace service::manager
   {
      struct State;

      namespace admin
      {
         namespace service::name
         {
            constexpr auto state = ".casual/service/state";

            namespace metric
            {
               constexpr auto reset = ".casual/service/metric/reset";
            } // metric

         } // service::name

         common::server::Arguments services( manager::State& state);

      } // admin

   } // service::manager
} // casual

