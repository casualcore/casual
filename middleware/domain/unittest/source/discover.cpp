//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/unittest/discover.h"
#include "domain/common.h"
#include "domain/discovery/api.h"

#include "common/communication/ipc.h"

namespace casual
{
   using namespace common;
   namespace domain::unittest
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


      namespace service
      {
         std::vector< std::string> discover( std::vector< std::string> services)
         {
            if( auto result = local::discover( std::move( services), {}))
               return algorithm::transform( result->content.services, []( auto& service){ return service.name;});

            return {};
         }
      } // service


      void discover( std::vector< std::string> services, std::vector< std::string> queues)
      {
         local::discover( std::move( services), std::move( queues));
      }

      
   } // domain::unittest
   
} // casual