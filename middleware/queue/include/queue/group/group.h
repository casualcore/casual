//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "queue/group/state.h"

#include "casual/platform.h"
#include "common/message/pending.h"

#include <string>

namespace casual
{
   namespace queue
   {
      namespace group
      {
         struct Settings
         {
            std::string queuebase;
            std::string name;
         };

         struct Server
         {
            Server( Settings settings);
            ~Server();

            void start();

         private:
            State m_state;
         };
      } // group

   } // queue

} // casual


