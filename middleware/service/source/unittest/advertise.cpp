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
         void advertise( std::vector< std::string> services)
         {
            message::service::Advertise message{ process::handle()};
            message.services.add = algorithm::transform( services, []( auto& service)
            {
               return message::service::advertise::Service{ std::move( service)};
            });

            communication::device::blocking::send( communication::instance::outbound::service::manager::device(), message);
         }

         void unadvertise( std::vector< std::string> services)
         {
            message::service::Advertise message{ process::handle()};
            message.services.remove = std::move( services);
            communication::device::blocking::send( communication::instance::outbound::service::manager::device(), message);

         }
      } // unittest
   } // common
} // casual