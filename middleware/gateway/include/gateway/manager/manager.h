//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_MANAGER_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_MANAGER_H_


#include "gateway/manager/state.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         struct Settings
         {

            std::string configuration;
         };


      } // manager

      class Manager
      {
      public:


         Manager( manager::Settings settings);

         ~Manager();

         void start();

      private:

         manager::State m_state;
      };

   } // gateway


} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_MANAGER_H_
