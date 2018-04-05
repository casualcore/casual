//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DOMAIN_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DOMAIN_H_


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
            bool no_auto_persist = false;

            inline void event( common::strong::ipc::id::value_type id) 
            {
               m_event = common::strong::ipc::id{ id};
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

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DOMAIN_H_
