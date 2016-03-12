//!
//! gateway.h
//!
//! Created on: Nov 8, 2015
//!     Author: Lazan
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_GATEWAY_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_GATEWAY_H_

#include "sf/namevaluepair.h"

#include <string>
#include <vector>

namespace casual
{
   namespace config
   {
      namespace gateway
      {

         struct Connection
         {
            std::string name;
            std::string type;
            std::string address;
            std::string restart;
            std::vector< std::string> services;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( type);
               archive & CASUAL_MAKE_NVP( address);
               archive & CASUAL_MAKE_NVP( restart);
               archive & CASUAL_MAKE_NVP( services);
            )
         };

         struct Gateway
         {
            std::string address;
            std::vector< gateway::Connection> connections;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( address);
               archive & CASUAL_MAKE_NVP( connections);
            )

         };


         Gateway get( const std::string& file);

         Gateway get();

      } // gateway
   } // config
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_GATEWAY_H_
