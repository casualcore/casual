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
#include "common/log.h"
#include "common/instance.h"

#include <string>

namespace casual
{
   namespace manager
   {
      template< typename F>
      struct basic_service
      {
         using function_type = F;

         template< typename A>
         auto operator () ( A&& argument) const
         {
            common::Trace trace{ "manager::service::basic_service::operator ()"};

            return function( std::move( argument));
         }

         std::string name;
         function_type function;
         common::service::visibility::Type visibility = common::service::visibility::Type::discoverable;
         std::string category;

         inline friend bool operator == ( const basic_service& lhs, const basic_service& rhs) { return lhs.name == rhs.name;}
         inline friend bool operator == ( const basic_service& lhs, std::string_view rhs) { return lhs.name == rhs;}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( name);
            CASUAL_SERIALIZE( visibility);
            CASUAL_SERIALIZE( category);
            
         )
      };

      namespace sequential
      {
         using Service = basic_service< common::function< service::invoke::Result( service::invoke::Parameter&&) const>>;

      } // sequential

      
      namespace concurrent
      {
         using Service = basic_service< common::function< void( service::invoke::concurrent::Parameter&&) const>>;

      } // concurrent

      using Service = std::variant< sequential::Service, concurrent::Service>;


      namespace service
      {
         std::string name( const Service& service);

         namespace advertise
         {
            common::message::service::advertise::Service transform( const sequential::Service& service);

            //! we advertise concurrent services as 'sequential', for now. It's not really clear what the 
            //! effects would be if we advertised them as concurrent.
            common::message::service::advertise::Service transform( const concurrent::Service& service);

            struct Result
            {
               std::optional< common::message::service::Advertise> sequential;
               std::optional< common::message::service::concurrent::Advertise> concurrent;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( sequential);
                  CASUAL_SERIALIZE( concurrent);
               )
            };

            inline Result transform( std::ranges::range auto&& services)
            {
               advertise::Result result;
               result.sequential.emplace( common::process::handle());
               result.sequential->alias = common::instance::alias();
               result.concurrent.emplace( common::process::handle());
               result.concurrent->alias = common::instance::alias();

               for( auto& service : services)
               {
                  if( auto sequential = std::get_if< sequential::Service>( &service))
                     result.sequential->services.add.push_back( transform( *sequential));
                  else if( auto concurrent = std::get_if< concurrent::Service>( &service))
                  {
                     // we advertise concurrent services as 'sequential', for now. It's not really clear what the
                     // effects would be if we advertised them as concurrent.
                     result.sequential->services.add.push_back( transform( *concurrent));
                  }
               }

               if( result.sequential->services.add.empty())
                  result.sequential = std::nullopt;
               if( result.concurrent->services.add.empty())
                  result.concurrent = std::nullopt;

               return result;

            }
            
         } // advertise

      } // service


   } // manager
} // casual
