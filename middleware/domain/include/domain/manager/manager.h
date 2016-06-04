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
            bool boot = false;
            bool bare = false;

         };


         class Manager
         {
         public:
            Manager( Settings&& settings);
            ~Manager();

            void start();

         private:

            common::file::scoped::Path m_singelton;
            State m_state;

         };

      } // manager
   } // domain


} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DOMAIN_H_
