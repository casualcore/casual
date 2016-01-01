//!
//! state.h
//!
//! Created on: Jan 1, 2016
//!     Author: Lazan
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_

#include "common/uuid.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {

         struct State
         {

            struct
            {
               common::Uuid id = common::uuid::make();

            } domain;

         };


      } // manager

   } // gateway
} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_
