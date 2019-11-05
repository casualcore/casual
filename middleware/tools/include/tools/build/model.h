//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "common/service/type.h"

#include <vector>
#include <string>

namespace casual
{
   namespace tools
   {
      namespace build
      {
         namespace model
         {
            struct Service
            {
               Service( std::string name) : name( name), function( std::move( name)) {}
               Service() = default;
               std::string name;
               std::string function;
               std::string category;
               common::service::transaction::Type transaction = common::service::transaction::Type::automatic;
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
               } paths;
            };
            
         } // model


      } // build
   } // tools
} // casual