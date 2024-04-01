//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/server/argument.h"

namespace casual
{
   namespace transaction::manager
   {
      struct State;
      namespace admin
      {
         namespace service::name
         {
            constexpr std::string_view state = ".casual/transaction/state";

            namespace scale
            {
               constexpr std::string_view instances = ".casual/transaction/scale/instances";
            } // update

         } // service::name

         common::server::Arguments services( manager::State& state);
      } // admin

   } // transaction::manager
} // casual

