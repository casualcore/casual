//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/manager/service/invoke.h"

#include "common/service/type.h"
#include "common/message/service.h"

#include "common/functional.h"

#include <string>

namespace casual
{
   namespace manager
   {
      struct Service
      {

         using function_type = std::function< service::invoke::Result( service::invoke::Parameter&&)>;


         service::invoke::Result operator () ( service::invoke::Parameter&& argument) const;

         std::string name;
         function_type function;

         common::service::visibility::Type visibility = common::service::visibility::Type::discoverable;
         std::string category;

         friend bool operator == ( const Service& lhs, const Service& rhs);
         friend bool operator == ( const Service& lhs, std::string_view rhs);

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( name);
            CASUAL_SERIALIZE( visibility);
            CASUAL_SERIALIZE( category);
            
         )

      };

      namespace service
      {
         //! Transform a service to a message::service::Advertise
         //! @param service the service to transform
         //! @return the service to be advertised
         common::message::service::advertise::Service transform( const Service& service);

      } // service
      
      
   } // manager

} // casual