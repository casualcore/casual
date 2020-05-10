//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/manager/state.h"

namespace casual
{
   namespace gateway
   {
      class Manager
      {
      public:
         Manager();
         ~Manager();

         void start();
      private:
         manager::State m_state;
      };

   } // gateway
} // casual

