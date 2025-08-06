//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/manager/service.h"

namespace casual
{
   using namespace common;

   namespace manager
   {
      namespace local
      {
         namespace
         {


            
         } // <unnamed>
      } // local
     

      namespace service
      {
         std::string name( const Service& service)
         {
            return std::visit( []( const auto& service) { return service.name; }, service);
         }



         namespace advertise
         {

            common::message::service::advertise::Service transform( const sequential::Service& service)
            {
               return {
                  .name = service.name,
                  .category = service.category,
                  .transaction = common::service::transaction::Type::none,
                  .visibility = service.visibility
               };
            }

            common::message::service::concurrent::advertise::Service transform( const concurrent::Service& service)
            {
               return {
                  .name = service.name,
                  .category = service.category,
                  .transaction = common::service::transaction::Type::none,
                  .visibility = service.visibility
               };
            }
            
         } // advertise

      } // service
   } // manager   
} // casual
