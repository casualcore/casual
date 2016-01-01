//!
//! manager.h
//!
//! Created on: Nov 8, 2015
//!     Author: Lazan
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
