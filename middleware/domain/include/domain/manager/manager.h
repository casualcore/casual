//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "domain/manager/state.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         struct Settings
         {
            std::vector< std::string> configurationfiles;

            bool bare = false;
            bool persist = false;

            struct
            {
               common::strong::ipc::id ipc;
               common::Uuid id;
            } event;
         };


         class Manager
         {
         public:
            Manager( Settings&& settings);
            ~Manager();

            void start();

         private:

            State m_state;
            common::file::scoped::Path m_singelton;

         };

      } // manager
   } // domain


} // casual


