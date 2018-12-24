//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "serviceframework/service/call.h"
#include "serviceframework/log.h"



namespace casual
{
   namespace serviceframework
   {
      namespace service
      {
         namespace call
         {
            Result invoke( const std::string& service, const payload_type& payload, Flags flags)
            {
               Trace trace{ "sf::service::call::invoke"};

               return common::service::call::context().sync( service, payload, flags);
            }
         } // call

         namespace send
         {
            descriptor_type invoke( const std::string& service, const payload_type& payload, Flags flags)
            {
               Trace trace{ "sf::service::call::send"};

               return common::service::call::context().async( service, payload, flags);
            }

         } // send

         namespace receive
         {
            Result invoke( descriptor_type descriptor, Flags flags)
            {
               Trace trace{ "sf::service::receive::send"};

               return common::service::call::context().reply( descriptor, flags);
            }
         } // receive


      } // service
   } // serviceframework
} // casual
