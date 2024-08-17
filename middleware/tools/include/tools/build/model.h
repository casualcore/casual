//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/service/type.h"
#include "common/serialize/macro.h"

#include <vector>
#include <string>

namespace casual
{
   namespace tools::build::model
   {
      struct Service
      {
         std::string name;
         std::string function{};
         std::string category{};
         common::service::transaction::Type transaction = common::service::transaction::Type::automatic;
         common::service::visibility::Type visibility = common::service::visibility::Type::discoverable;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( name);
            CASUAL_SERIALIZE( function);
            CASUAL_SERIALIZE( category);
            CASUAL_SERIALIZE( transaction);
            CASUAL_SERIALIZE( visibility);
         )
      };

      struct Resource
      {
         std::string key;
         std::string name;
         std::string xa_struct_name;

         std::vector< std::string> libraries;

         struct
         {
            std::vector< std::string> include;
            std::vector< std::string> library;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( include);
               CASUAL_SERIALIZE( library);
            )
         } paths;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( key);
            CASUAL_SERIALIZE( name);
            CASUAL_SERIALIZE( xa_struct_name);
            CASUAL_SERIALIZE( libraries);
            CASUAL_SERIALIZE( paths);
         )

      };
            
   } // tools::build::model
} // casual
