//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "server/service.h"



namespace casual
{
   namespace server
   {

      service::invoke::Result Service::operator () ( service::invoke::Parameter&& argument)
      {
         return std::invoke( function, std::move( argument));
      }

      bool operator == ( const Service& lhs, const Service& rhs)
      {
         return lhs == rhs.compare;
      }

      bool operator == ( const Service& lhs, const void* rhs)
      {
         if( lhs.compare && rhs)
            return lhs.compare == rhs;
         
         return false;
      }

      bool operator == ( const Service& lhs, const std::string& rhs)
      {
         return lhs.name == rhs;
      }

   } // server
} // casual
