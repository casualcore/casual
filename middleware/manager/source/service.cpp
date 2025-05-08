//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/manager/service.h"

#include "common/log.h"


namespace casual
{
   using namespace common;

   namespace manager
   {
      service::invoke::Result Service::operator () ( service::invoke::Parameter&& argument) const
      {
         Trace trace{ "manager::service::Service::operator ()"};

         return function( std::move( argument));
      }

      bool operator == ( const Service& lhs, const Service& rhs)
      {
         return lhs.name == rhs.name;
      }
      
      bool operator == ( const Service& lhs, std::string_view rhs)
      {
         return lhs.name == rhs;
      }

      namespace service
      {
         common::message::service::advertise::Service transform( const Service& service)
         {
            common::message::service::advertise::Service result;
            result.name = service.name;
            result.category = service.category;
            result.transaction = decltype( result.transaction)::none;
            result.visibility = service.visibility;

            return result;
         }

      } // service
      
   } // manager
   
} // casual