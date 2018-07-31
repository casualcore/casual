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

            std::vector< std::string> events;

            bool bare = false;
            bool persist = false;

            inline void event( const std::string& id) 
            {
               m_event = common::strong::ipc::id{ common::Uuid{ id}};
            }
            auto event() const { return m_event;};


         private:
            common::strong::ipc::id m_event;
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


