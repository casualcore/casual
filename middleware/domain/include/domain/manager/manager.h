//!
//! casual 
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

            common::platform::ipc::handle::type event_queue = 0;
            std::vector< std::string> events;

            bool bare = false;
            bool no_auto_persist = false;

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
