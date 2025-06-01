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

#include "service/protocol/call.h"

namespace casual
{
   namespace service::unittest
   {
      namespace local
      {
         namespace
         {
            namespace ipc
            {
               auto& manager() { return common::communication::instance::outbound::service::manager::device();}
            } // ipc
         } // <unnamed>
      } // local

      void advertise( std::vector< std::string> services)
      {
         advertise( services, common::process::handle());
      }

      void advertise( std::vector< std::string> services, const common::process::Handle& handle)
      {
         common::message::service::Advertise message{ handle};
         message.alias = common::instance::alias();
         message.services.add = common::algorithm::transform( services, []( auto& service)
         {
            return common::message::service::advertise::Service{ .name = std::move( service)};
         });

         common::communication::device::blocking::send( local::ipc::manager(), message);
      }

      void unadvertise( std::vector< std::string> services)
      {
         common::message::service::Advertise message{ common::process::handle()};
         message.alias = common::instance::alias();
         message.services.remove = std::move( services);
         common::communication::device::blocking::send( local::ipc::manager(), message);
      }

      namespace concurrent
      {
         void advertise( std::vector< std::string> services)
         {
            common::message::service::concurrent::Advertise message{ common::process::handle()};
            message.alias = common::instance::alias();
            message.services.add = common::algorithm::transform( services, []( auto& service)
            {
               return common::message::service::concurrent::advertise::Service{ std::move( service), "remote", common::service::transaction::Type::automatic, common::service::visibility::Type::discoverable};
            });

            common::communication::device::blocking::send( local::ipc::manager(), message);
         }

         void unadvertise( std::vector< std::string> services)
         {
            common::message::service::concurrent::Advertise message{ common::process::handle()};
            message.alias = common::instance::alias();
            message.services.remove = std::move( services);
            common::communication::device::blocking::send( local::ipc::manager(), message);
         }
      } // concurrent


      common::message::service::lookup::Reply lookup( std::string service)
      {
         common::Trace trace{ "service::unittest::lookup"};

         common::message::service::lookup::Request lookup{ common::process::handle()};
         lookup.requested = std::move( service);
         lookup.context.semantic = decltype( lookup.context.semantic)::regular;
         
         return common::communication::ipc::call( local::ipc::manager(), lookup);
      }

      manager::admin::model::State state()
      {
         common::unittest::service::wait::until::advertised( manager::admin::service::name::state);

         casual::service::protocol::binary::Call call;

         auto reply = call( manager::admin::service::name::state);

         return reply.extract< manager::admin::model::State>();
      }

      namespace send
      {
         namespace wait
         {
            auto request( std::string service, platform::binary::type payload) -> common::strong::correlation::id
            {
               common::Trace trace{ "service::unittest::send::wait::request"};

               common::message::service::lookup::Request lookup{ common::process::handle()};
               lookup.requested = std::move( service);
               lookup.context.semantic = decltype( lookup.context.semantic)::wait;

               auto lookup_reply = common::communication::ipc::call( local::ipc::manager(), lookup);

               if( lookup_reply.state != decltype( lookup_reply.state)::idle)
                  common::code::raise::error( common::code::casual::internal_unexpected_value, "failed to lookup service: ", lookup.requested, " - ", lookup_reply.state);

               {
                  common::message::service::call::callee::Request message{ common::process::handle()};
                  message.correlation = lookup_reply.correlation;
                  message.service = std::move( lookup_reply.service);
                  message.buffer.data = std::move( payload);
                  message.buffer.type = common::buffer::type::x_octet;

                  return common::communication::device::blocking::send( lookup_reply.process.ipc, message);
               }

            }
            
         } // wait

         void ack( const common::message::service::call::callee::Request& request)
         {
            common::message::service::call::ACK message;
            message.correlation = request.correlation;
            message.metric.pending = request.pending;
            message.metric.service = request.service.name;
            message.metric.trid = request.trid;

            message.metric.process = common::process::handle();
            
            common::communication::device::blocking::send( local::ipc::manager(), message);
         }

         void ack( const common::message::service::lookup::Reply& lookup)
         {
            common::message::service::call::ACK message;
            message.correlation = lookup.correlation;
            message.metric.pending = lookup.pending;
            message.metric.service = lookup.service.name;

            message.metric.process = common::process::handle();
            
            common::communication::device::blocking::send( local::ipc::manager(), message);

         }
      } // send

      namespace server
      {
         common::strong::correlation::id echo( const common::strong::correlation::id& correlation)
         {
            auto request = common::communication::ipc::receive< common::message::service::call::callee::Request>( correlation);

            send::ack( request);

            auto reply = common::message::reverse::type( request);
            reply.buffer = std::move( request.buffer);

            return common::communication::device::blocking::send( request.process.ipc, reply);
         }
         
      } // server

   } // common::unittest
} // casual