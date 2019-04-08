//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/message/service.h"
#include "common/message/pending.h"

#include <map>
#include <vector>
#include <deque>
#include <string>


namespace casual
{
   namespace service
   {
      namespace forward
      {

         struct State
         {
            std::unordered_map< std::string, std::deque< common::message::service::call::callee::Request>> requested;
         };

         struct Cache
         {
            Cache();
            ~Cache();

            void start();

         private:
            State m_state;

         };

      } // forward
   } // service
} // casual


