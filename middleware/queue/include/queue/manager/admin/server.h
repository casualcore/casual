//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "queue/manager/admin/model.h"

#include "common/server/argument.h"

namespace casual
{
   namespace queue
   {
      namespace manager
      {
         struct State;

         namespace admin
         {
            common::server::Arguments services( manager::State& state);

         } // manager
      } // manager
   } // queue


} // casual


