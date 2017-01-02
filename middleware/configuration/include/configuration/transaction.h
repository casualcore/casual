//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_TRANSACTION_H_
#define CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_TRANSACTION_H_

#include "sf/namevaluepair.h"
#include "sf/platform.h"

#include <string>
#include <vector>

namespace casual
{
   namespace configuration
   {
      namespace transaction
      {
         struct Manager
         {
            std::string path = "casual-transaction-manager";
            std::string database = "${CASUAL_DOMAIN_HOME}/transaction/log.db";

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( path);
               archive & CASUAL_MAKE_NVP( database);
            )
         };


         struct Resource
         {
            std::string key;
            std::string name;
            sf::optional< std::size_t> instances;

            std::string openinfo;
            std::string closeinfo;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( key);
               archive & CASUAL_MAKE_NVP( name);
               archive & CASUAL_MAKE_NVP( instances);
               archive & CASUAL_MAKE_NVP( openinfo);
               archive & CASUAL_MAKE_NVP( closeinfo);
            )
         };

         struct Transaction
         {
            Manager manager;
            std::vector< Resource> resources;

            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( manager);
               archive & CASUAL_MAKE_NVP( resources);
            )
         };


      } // transaction
   } // configuration
} // casual

#endif // CASUAL_MIDDLEWARE_CONFIGURATION_INCLUDE_CONFIG_TRANSACTION_H_
