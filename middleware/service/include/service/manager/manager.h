//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "service/manager/state.h"

#include <string>


namespace casual
{
   namespace service
   {
      namespace manager
      {
         struct Settings
         {
            std::string forward;
         };
      } // manager


      class Manager
      {
      public:

         Manager( manager::Settings&& settings);
         ~Manager();

         void start();

         inline const manager::State& state() const { return m_state; }

      private:
         manager::State m_state;
      };

   } // service
} // casual





