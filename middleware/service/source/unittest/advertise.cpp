//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "service/unittest/advertise.h"

#include "common/message/service.h"
#include "common/communication/instance.h"

namespace casual
{
   using namespace common;
   namespace service
   {
      namespace unittest
      {
         namespace local
         {
            namespace
            {
               void call( 
                  const process::Handle& handle, 
                  std::vector< message::service::advertise::Service> services,
                  message::service::Advertise::Directive directive = message::service::Advertise::Directive::add)
               {
                   message::service::Advertise message;
                   message.process = handle;
                   message.services = std::move( services);
                   message.directive = directive;

                   communication::ipc::blocking::send( communication::instance::outbound::service::manager::device(), message);
               }

            } // <unnamed>
         } // local
         void advertise( std::vector< std::string> services)
         {
            local::call( process::handle(), algorithm::transform( services, []( auto& s)
            {
               return message::service::advertise::Service{ std::move( s)};
            }));
         }

         void unadvertise( std::vector< std::string> services)
         {
            local::call( process::handle(), algorithm::transform( services, []( auto& s)
            {
               return message::service::advertise::Service{ std::move( s)};
            }), message::service::Advertise::Directive::remove);
         }
      } // unittest
   } // common
} // casual