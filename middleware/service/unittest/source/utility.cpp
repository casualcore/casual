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

      namespace wait::until
      {
         void advertised( std::string_view service)
         {
            auto lookup = [ service]()
            {
               common::message::service::lookup::Request request{ common::process::handle()};
               request.requested = service;

               auto reply = common::communication::ipc::call( local::ipc::manager(), request);

               if( reply.absent())
                  return false;
               
               // the service is advertised, we need to discard the reservation.
               {
                  common::message::service::lookup::discard::Request discard{ common::process::handle()};
                  discard.correlation = reply.correlation;
                  discard.reply = false;
                  common::communication::device::blocking::send( local::ipc::manager(), discard);
               }
               
               return true;
            };
            
            // we try a bunch of times, but we don't wait forever.
            common::unittest::eventually::succeed( lookup);
         }

      } // wait::until

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
         wait::until::advertised( manager::admin::service::name::state);

         casual::service::protocol::binary::Call call;

         auto reply = call( manager::admin::service::name::state);

         return reply.extract< manager::admin::model::State>();
      }

      namespace send
      {
         [[nodiscard]] common::strong::correlation::id request( std::string service, platform::binary::type payload, const common::transaction::ID& trid)
         {
            auto send_lookup = []( auto service, auto& trid){
               common::message::service::lookup::Request request{ common::process::handle()};
               request.trid = trid;
               request.requested = std::move( service);
               request.context.semantic = decltype( request.context.semantic)::regular;
               return common::communication::device::blocking::send( local::ipc::manager(), request);
            };
            
            auto lookup = common::communication::ipc::receive< common::message::service::lookup::Reply>( send_lookup( std::move( service), trid));
            
            if( lookup.state == decltype( lookup.state)::absent)
               common::code::raise::error( common::code::xatmi::no_entry);
            if( lookup.state == decltype( lookup.state)::timeout)
               common::code::raise::error( common::code::xatmi::timeout);

            common::message::service::call::callee::Request message{ common::process::handle()};
            message.correlation = lookup.correlation;
            message.service = std::move( lookup.service);
            message.trid = trid;
            message.buffer.data = std::move( payload);
            message.buffer.type = common::buffer::type::x_octet;

            return common::communication::device::blocking::send( lookup.process.ipc, message);

         }

         common::strong::correlation::id request( std::string service, platform::binary::type payload)
         {
            return request( std::move( service), std::move( payload), {});
         }

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

      platform::binary::type receive( const common::strong::correlation::id& correlation)
      {
         auto request = common::communication::ipc::receive< common::message::service::call::Reply>( correlation);
         return request.buffer.data;
      }

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