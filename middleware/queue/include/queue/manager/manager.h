//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "queue/manager/state.h"


#include "common/message/queue.h"
#include "common/transaction/id.h"



#include <vector>

namespace casual
{
   namespace queue
   {
      namespace manager
      {
         struct Settings
         {
            std::string group_executable;
         };

         std::vector< common::message::queue::information::queues::Reply> queues( State& state);
         common::message::queue::information::messages::Reply messages( State& state, const std::string& queue);
      } // manager

      struct Manager
      {
         Manager( manager::Settings settings);
         ~Manager();

         void start();

      private:
         manager::State m_state;
      };

   } // queue
} // casual


