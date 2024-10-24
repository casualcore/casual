//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "service/unittest/utility.h"
#include "service/manager/admin/server.h"

#include "common/communication/instance.h"
#include "common/instance.h"
#include "common/unittest.h"

#include "serviceframework/service/protocol/call.h"

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
            return message::service::advertise::Service{ .name = std::move( service)};
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

      namespace concurrent
      {
         void advertise( std::vector< std::string> services)
         {
            message::service::concurrent::Advertise message{ process::handle()};
            message.alias = instance::alias();
            message.services.add = algorithm::transform( services, []( auto& service)
            {
               return message::service::concurrent::advertise::Service{ std::move( service), "remote", common::service::transaction::Type::automatic, common::service::visibility::Type::discoverable};
            });

            communication::device::blocking::send( local::ipc::manager(), message);
         }

         void unadvertise( std::vector< std::string> services)
         {
            message::service::concurrent::Advertise message{ process::handle()};
            message.alias = instance::alias();
            message.services.remove = std::move( services);
            communication::device::blocking::send( local::ipc::manager(), message);
         }
      } // concurrent

      manager::admin::model::State state()
      {
         common::unittest::service::wait::until::advertised( manager::admin::service::name::state);

         serviceframework::service::protocol::binary::Call call;

         auto reply = call( manager::admin::service::name::state);

         return reply.extract< manager::admin::model::State>();
      }

      namespace send
      {
         void ack( const message::service::call::callee::Request& request)
         {
            message::service::call::ACK message;
            message.correlation = request.correlation;
            message.metric.pending = request.pending;
            message.metric.service = request.service.name;
            message.metric.trid = request.trid;

            message.metric.process = process::handle();
            
            communication::device::blocking::send( local::ipc::manager(), message);
         }

         void ack( const message::service::lookup::Reply& lookup)
         {
            message::service::call::ACK message;
            message.correlation = lookup.correlation;
            message.metric.pending = lookup.pending;
            message.metric.service = lookup.service.name;

            message.metric.process = process::handle();
            
            communication::device::blocking::send( local::ipc::manager(), message);

         }
      } // send

      namespace server
      {
         common::strong::correlation::id echo( const common::strong::correlation::id& correlation)
         {
            auto request = communication::ipc::receive< common::message::service::call::callee::Request>( correlation);

            send::ack( request);

            auto reply = message::reverse::type( request);
            reply.buffer = std::move( request.buffer);

            return communication::device::blocking::send( request.process.ipc, reply);
         }
         
      } // server

   } // common::unittest
} // casual