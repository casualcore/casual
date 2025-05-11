//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/unittest/discover.h"
#include "domain/common.h"
#include "domain/discovery/api.h"
#include "domain/discovery/admin/server.h"

#include "common/unittest.h"
#include "common/communication/ipc.h"

#include "serviceframework/service/protocol/call.h"

namespace casual
{
   using namespace common;
   namespace domain::unittest::discover
   {
      namespace local
      {
         namespace
         {
            std::optional< message::discovery::api::Reply> discover( std::vector< std::string> services, std::vector< std::string> queues)
            {
               if( auto correlation = casual::domain::discovery::request( std::move( services), std::move( queues)))
                  return communication::ipc::receive< message::discovery::api::Reply>( correlation);

               return {};

            }
         } // <unnamed>
      } // local

      casual::domain::discovery::admin::model::State state()
      {
         common::unittest::service::wait::until::advertised( casual::domain::discovery::admin::service::name::state);
         serviceframework::service::protocol::binary::Call call;
         auto reply = call( casual::domain::discovery::admin::service::name::state);
         return reply.extract< casual::domain::discovery::admin::model::State>();
      }


      std::vector< std::string> services( std::vector< std::string> services)
      {
         if( auto result = local::discover( std::move( services), {}))
            return algorithm::transform( result->content.services, []( auto& service){ return service.name;});

         return {};
      }


      void request( std::vector< std::string> services, std::vector< std::string> queues)
      {
         local::discover( std::move( services), std::move( queues));
      }


      namespace fetch
      {
         namespace predicate
         {
            auto provider( message::discovery::api::provider::registration::Ability ability, platform::size::type count) -> common::unique_function< bool( const casual::domain::discovery::admin::model::State&)>
            {
               return [ ability, count]( const casual::domain::discovery::admin::model::State& state)
               {
                  return algorithm::count_if( state.providers, [ ability]( auto& provider)
                  {
                     return ( provider.abilities & ability) == ability;
                  }) >= count;
               };
            }
            
         } // predicate
      } // fetch

      
   } // domain::unittest::discover
   
} // casual