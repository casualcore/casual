//!
//! gateway.h
//!
//! Created on: Nov 8, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_GATEWAY_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_GATEWAY_H_

#include "sf/namevaluepair.h"

namespace casual
{
   namespace config
   {
      namespace gateway
      {

         struct Listener
         {
            std::string port;
            std::string ip;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( port);
               archive & CASUAL_MAKE_NVP( ip);
            )
         };


      } // gateway

      struct Gateway
      {
         std::vector< gateway::Listener> listernes;

         CASUAL_CONST_CORRECT_SERIALIZE
         (
            archive & CASUAL_MAKE_NVP( listernes);
         )

      };

   } // config
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_GATEWAY_H_
