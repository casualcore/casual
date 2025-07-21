//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "service/invoke.h"

#include "common/service/type.h"
#include "common/string.h"

#include <functional>
#include <string>
#include <ostream>
#include <cstdint>

namespace casual
{
   namespace server
   {
      namespace service
      {
         using namespace common::service;
         
      } // service

      struct Service
      {
         using function_type = std::function< service::invoke::Result( service::invoke::Parameter&&)>;

         service::invoke::Result operator () ( service::invoke::Parameter&& argument);

         std::string name;
         function_type function;

         service::transaction::Type transaction = service::transaction::Type::automatic;
         service::visibility::Type visibility = service::visibility::Type::discoverable;
         std::string category;

         //! Only to be able to compare 'c-functions', which we have to do according to the XATMI-spec
         const void* compare = nullptr;

         friend bool operator == ( const Service& lhs, const Service& rhs);
         friend bool operator == ( const Service& lhs, const void* rhs);
         friend bool operator == ( const Service& lhs, const std::string& rhs);

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( name);
            CASUAL_SERIALIZE( transaction);
            CASUAL_SERIALIZE( visibility);
            CASUAL_SERIALIZE( category);
            CASUAL_SERIALIZE( compare);
            
         )
      };

   } // server
} // casual


