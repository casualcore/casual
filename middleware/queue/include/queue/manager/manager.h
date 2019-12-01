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
            struct
            {
               std::string executable;
            } group;
         };

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


