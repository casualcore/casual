//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DOMAIN_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DOMAIN_H_


#include "domain/state.h"


namespace casual
{
   namespace domain
   {

      struct Settings
      {

         std::vector< std::string> configurationfiles;

      };


      class Domain
      {
      public:
         Domain( Settings&& settings);

         void start();

      private:

         common::file::scoped::Path m_singelton;

      };

   } // domain


} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_DOMAIN_H_
