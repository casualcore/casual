//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "server/handle/policy.h"

#include "transaction/context.h"

#include "service/call/context.h"
#include "service/lookup.h"

#include "common/buffer/pool.h"

#include "common/communication/instance.h"
#include "common/instance.h"

#include "common/log.h"
#include "common/execute.h"

namespace casual
{
   namespace server::handle::policy
   {
      namespace local
      {
         namespace
         {
            void advertise( std::vector< common::message::service::advertise::Service> services)
            {
               common::Trace trace{ "common::server::local::advertise"};

               if( ! services.empty())
               {
                  common::message::service::Advertise advertise{ common::process::handle()};
                  advertise.alias = common::instance::alias();
                  advertise.services.add = std::move( services);

                  common::log::debug( "advertise: ", advertise);

                  common::signal::thread::scope::Mask block{ common::signal::set::filled( common::code::signal::terminate, common::code::signal::interrupt)};

                  common::communication::device::blocking::send( common::communication::instance::outbound::service::manager::device(), advertise);

                  common::log::line( common::log::category::event::server, "advertise");
               }
            }

         } // <unnamed>
      } // local

      void advertise( std::vector< server::Service> services)
      {
         local::advertise( common::algorithm::transform( services, []( auto& service)
         {
            common::message::service::advertise::Service result;
            result.name = service.name;
            result.category = service.category;
            result.transaction = service.transaction;
            result.visibility = service.visibility;

            return result;
         }));
      }

      namespace call
      {

         void Default::configure( server::Arguments&& arguments)
         {
            common::Trace trace{ "server::handle::policy::Default::configure"};
            common::log::debug( "arguments: ", arguments);

            // Connection to the domain has been done before...

            // configure resources, if any.
            transaction::Context::instance().configure( arguments.resources);

            // Let the service-manager know about our services...
            policy::advertise( std::move( arguments.services));

            common::log::line( common::log::category::event::server, "configure");
         }

         void Default::reply( common::strong::ipc::id id, common::message::service::call::Reply& message)
         {
            common::Trace trace{ "server::handle::policy::Default::reply"};
            common::log::debug( "ipc: ", id, "reply: ", message);

            common::communication::device::blocking::send( id, message);

            common::log::line( common::log::category::event::service, "reply");
         }

         void Default::reply( common::strong::ipc::id id, common::message::conversation::callee::Send& message)
         {
            common::Trace trace{ "server::handle::policy::Default::conversation::reply"};
            common::log::debug( "ipc: ", id, "reply: ", message);

            common::communication::device::blocking::send( id, message);

            common::log::line( common::log::category::event::service, "reply");
         }

         void Default::ack( const common::message::service::call::ACK& message)
         {
            common::Trace trace{ "server::handle::policy::Default::ack"};

            common::log::debug( "reply: ", message);

            common::communication::device::blocking::send( common::communication::instance::outbound::service::manager::device(), message);

            common::log::line( common::log::category::event::service, "ack");
         }

         void Default::statistics( common::strong::ipc::id id, common::message::event::service::Call& event)
         {
            common::Trace trace{ "server::handle::policy::Default::statistics"};

            common::log::debug( "event:", event);

            try
            {
               common::communication::device::blocking::send( id, event);
            }
            catch( ...)
            {
               common::log::error( common::exception::capture());
            }

            common::log::line( common::log::category::event::service, "statistics");
         }

         void Default::transaction(
               const common::transaction::ID& trid,
               const server::Service& service)
         {
            common::Trace trace{ "server::handle::policy::Default::transaction"};

            common::log::debug( "trid: ", trid, " - service: ", service);

            // We keep track of callers transaction (can be null-trid).
            transaction::context().caller = trid;

            switch( service.transaction)
            {
               case service::transaction::Type::automatic:
               {
                  if( trid)
                     transaction::Context::instance().join( trid);
                  else
                     transaction::Context::instance().start();

                  break;
               }
               case service::transaction::Type::branch:
               {
                  if( trid)
                     transaction::Context::instance().branch( trid);
                  else
                     transaction::Context::instance().start();
                  break;
               }
               case service::transaction::Type::join:
               {
                  transaction::Context::instance().join( trid);
                  break;
               }
               case service::transaction::Type::atomic:
               {
                  transaction::Context::instance().start();
                  break;
               }
               default:
               {
                  common::log::line( common::log::category::error, "unknown transaction semantics for service: ", service);
                  // fallthrough
               }
               case service::transaction::Type::none:
               {
                  // We don't start or join any transactions
                  // (technically we join a null-trid)
                  transaction::Context::instance().join( transaction::ID{ common::process::id()});
                  break;
               }
            }
         }


         common::message::service::transaction::State Default::transaction( bool commit)
         {
            // try to wait for all in-flights that are associated with transactions
            casual::service::call::context().finalize( transaction::context().associated());

            return transaction::context().finalize( commit);
         }


         namespace local
         {
            namespace
            {
               template< typename M>
               void forward( M&& message, service::invoke::Forward&& forward)
               {
                  common::Trace trace{ "server::handle::policy::local::forward"};


                  if( transaction::context().pending())
                     common::code::raise::error( common::code::xatmi::service_error, "forward with pending transactions - service: ", message.service.name);

                  casual::service::Lookup lookup{
                     forward.parameter.service.name,
                     {}, // nill trid
                     decltype( casual::service::lookup::Context::semantic)::no_reply};

                  // TODO make this forward work without a copy of payload...
                  auto request = message;

                  auto target = casual::service::lookup::reply( std::move( lookup));

                  request.buffer = std::move( forward.parameter.payload);
                  request.service = target.service;

                  common::log::debug( "policy::Default::forward - request:", request);

                  common::communication::device::blocking::send( target.process.ipc, request);
               }

            } // <unnamed>
         } // local

         void Default::forward( service::invoke::Forward&& forward, const common::message::service::call::callee::Request& message)
         {
            local::forward( message, std::move( forward));
         }


         void Default::forward( service::invoke::Forward&& forward, const common::message::conversation::connect::callee::Request& message)
         {
            local::forward( message, std::move( forward));
         }


         void Admin::configure( server::Arguments&& arguments)
         {
            // Connection to the domain has been done before...

            if( ! arguments.resources.empty())
               common::code::raise::error( common::code::casual::invalid_semantics, "can't build and link an administration server with resources");

            policy::advertise( std::move( arguments.services));

         }

         void Admin::reply( common::strong::ipc::id id, common::message::service::call::Reply& message)
         {
            common::communication::device::blocking::send( id, message);
         }

         void Admin::ack( const common::message::service::call::ACK& message)
         {
            common::communication::device::blocking::send( common::communication::instance::outbound::service::manager::device(), message);
         }

         void Admin::statistics( common::strong::ipc::id id, common::message::event::service::Call& event)
         {
            // no-op
         }

         void Admin::transaction(
               const common::transaction::ID& trid,
               const server::Service& service)
         {
            // no-op
         }

         common::message::service::transaction::State Admin::transaction( bool commit)
         {
            // no-op
            return {};
         }

         void Admin::forward( service::invoke::Forward&& forward, const common::message::service::call::callee::Request& message)
         {
            common::code::raise::error( common::code::casual::invalid_semantics, "can't forward within an administration server");
         }

      } // call

   } // server::handle::policy
} // casual
