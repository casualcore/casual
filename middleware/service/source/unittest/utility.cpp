//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "service/unittest/utility.h"

#include "common/communication/instance.h"
#include "common/instance.h"

namespace casual
{
   using namespace common;
   namespace service::unittest
   {
      namespace local
      {
         namespace
         {
            namespace ipc
            {
               auto& manager() { return communication::instance::outbound::service::manager::device();}
            } // ipc
         } // <unnamed>
      } // local

      void advertise( std::vector< std::string> services)
      {
         message::service::Advertise message{ process::handle()};
         message.alias = instance::alias();
         message.services.add = algorithm::transform( services, []( auto& service)
         {
            return message::service::advertise::Service{ std::move( service)};
         });

         communication::device::blocking::send( local::ipc::manager(), message);
      }

      void unadvertise( std::vector< std::string> services)
      {
         message::service::Advertise message{ process::handle()};
         message.alias = instance::alias();
         message.services.remove = std::move( services);
         communication::device::blocking::send( local::ipc::manager(), message);
      }

      namespace send
      {
         void ack( const message::service::call::callee::Request& request)
         {
            message::service::call::ACK message;
            message.metric.pending = request.pending;
            message.metric.service = request.service.name;
            message.metric.type = ( request.service.type == decltype( request.service.type)::concurrent) ?
               decltype( message.metric.type)::concurrent : decltype( message.metric.type)::sequential;

            message.metric.process = process::handle();
            
            communication::device::blocking::send( local::ipc::manager(), message);
         }
      } // send

   } // common::unittest
} // casual