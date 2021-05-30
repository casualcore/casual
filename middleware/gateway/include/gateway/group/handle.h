//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/ipc.h"

#include "common/algorithm.h"
#include "common/process.h"

namespace casual
{
   namespace gateway::group::handle
   {
      namespace signal::process
      {
         inline auto exit()
         {
            return []()
            {
               common::algorithm::for_each( common::process::lifetime::ended(), []( auto& exit)
               {
                  ipc::inbound().push( common::message::event::process::Exit{ exit});
               });
            };
         }
      } // signal::process
   } // gateway::group::handle
} // casual