//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "sf/service/call.h"
#include "sf/log.h"



namespace casual
{
   namespace sf
   {
      namespace service
      {
         namespace call
         {
            Result invoke( const std::string& service, const payload_type& paylaod, Flags flags)
            {
               sf::Trace trace{ "sf::service::call::invoke"};

               return common::service::call::context().sync( service, paylaod, flags);
            }
         } // call

         namespace send
         {
            descriptor_type invoke( const std::string& service, const payload_type& paylaod, Flags flags)
            {
               sf::Trace trace{ "sf::service::call::send"};

               return common::service::call::context().async( service, paylaod, flags);
            }

         } // send

         namespace receive
         {
            Result invoke( descriptor_type descriptor, Flags flags)
            {
               sf::Trace trace{ "sf::service::receive::send"};

               return common::service::call::context().reply( descriptor, flags);
            }
         } // receive


      } // service
   } // sf
} // casual
