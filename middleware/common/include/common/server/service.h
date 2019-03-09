//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/service/type.h"
#include "common/service/invoke.h"

#include <xatmi/defines.h>

#include <functional>
#include <string>
#include <ostream>
#include <cstdint>

namespace casual
{
   namespace common
   {
      namespace server
      {

         struct Service
         {

            using function_type = std::function< service::invoke::Result( service::invoke::Parameter&&)>;

            Service( std::string name, function_type function, service::transaction::Type transaction, std::string category);
            Service( std::string name, function_type function);

            service::invoke::Result operator () ( service::invoke::Parameter&& argument);

            std::string name;
            function_type function;

            service::transaction::Type transaction = service::transaction::Type::automatic;
            std::string category;

            //! Only to be able to compare 'c-functions', which we have to do according to the XATMI-spec
            const void* compare = nullptr;

            friend std::ostream& operator << ( std::ostream& out, const Service& service);

            friend bool operator == ( const Service& lhs, const Service& rhs);
            friend bool operator == ( const Service& lhs, const void* rhs);
            friend bool operator != ( const Service& lhs, const Service& rhs);

         };

         namespace xatmi
         {
            using function_type = std::function< void( TPSVCINFO*)>;

            server::Service service( std::string name, function_type function, service::transaction::Type transaction, std::string category);
            server::Service service( std::string name, function_type function);

            const void* address( const function_type& function);

         } // xatmi

      } // server
   } // common
} // casual


