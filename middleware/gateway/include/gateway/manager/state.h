//!
//! state.h
//!
//! Created on: Jan 1, 2016
//!     Author: Lazan
//!

#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_

#include "common/process.h"

namespace casual
{
   namespace gateway
   {
      namespace manager
      {
         namespace state
         {



         } // state

         struct State
         {

            struct Connection
            {

               common::process::Handle process;
            };



            std::vector< Connection> connections;
         };


      } // manager

   } // gateway
} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_MANAGER_STATE_H_
