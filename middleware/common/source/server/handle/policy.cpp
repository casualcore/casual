//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/server/handle/policy.h"

#include "common/transaction/context.h"
#include "common/buffer/pool.h"
#include "common/service/lookup.h"
#include "common/communication/instance.h"
#include "common/instance.h"

#include "common/log.h"
#include "common/execute.h"

namespace casual
{
   namespace common::server::handle::policy
   {
      namespace local
      {
         namespace
         {
            void advertise( std::vector< message::service::advertise::Service> services)
            {
               Trace trace{ "common::server::local::advertise"};

               if( ! services.empty())
               {
                  message::service::Advertise advertise{ process::handle()};
                  advertise.alias = instance::alias();
                  advertise.services.add = std::move( services);

                  log::line( log::debug, "advertise: ", advertise);

                  signal::thread::scope::Mask block{ signal::set::filled( code::signal::terminate, code::signal::interrupt)};

                  communication::device::blocking::send( communication::instance::outbound::service::manager::device(), advertise);

                  log::line( log::category::event::server, "advertise");
               }
            }

         } // <unnamed>
      } // local

      void advertise( std::vector< server::Service> services)
      {
         local::advertise( algorithm::transform( services, []( auto& service)
         {
            message::service::advertise::Service result;
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
            Trace trace{ "server::handle::policy::Default::configure"};
            log::line( verbose::log, "arguments: ", arguments);

            // Connection to the domain has been done before...

            // configure resources, if any.
            transaction::Context::instance().configure( arguments.resources);

            // Let the service-manager know about our services...
            policy::advertise( std::move( arguments.services));

            log::line( log::category::event::server, "configure");
         }

         void Default::reply( strong::ipc::id id, message::service::call::Reply& message)
         {
            Trace trace{ "server::handle::policy::Default::reply"};
            log::line( log::debug, "ipc: ", id, "reply: ", message);

            communication::device::blocking::send( id, message);

            log::line( log::category::event::service, "reply");
         }

         void Default::reply( strong::ipc::id id, message::conversation::callee::Send& message)
         {
            Trace trace{ "server::handle::policy::Default::conversation::reply"};
            log::line( log::debug, "ipc: ", id, "reply: ", message);

            communication::device::blocking::send( id, message);

            log::line( log::category::event::service, "reply");
         }

         void Default::ack( const message::service::call::ACK& message)
         {
            Trace trace{ "server::handle::policy::Default::ack"};

            log::line( verbose::log, "reply: ", message);

            communication::device::blocking::send( communication::instance::outbound::service::manager::device(), message);

            log::line( log::category::event::service, "ack");
         }

         void Default::statistics( strong::ipc::id id, message::event::service::Call& event)
         {
            Trace trace{ "server::handle::policy::Default::statistics"};

            log::line( log::debug, "event:", event);

            try
            {
               communication::device::blocking::send( id, event);
            }
            catch( ...)
            {
               log::line( log::category::error, exception::capture());
            }

            log::line( log::category::event::service, "statistics");
         }

         void Default::transaction(
               const common::transaction::ID& trid,
               const server::Service& service)
         {
            Trace trace{ "server::handle::policy::Default::transaction"};

            log::line( log::debug, "trid: ", trid, " - service: ", service);

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
                  log::line( log::category::error, "unknown transaction semantics for service: ", service);
                  // fallthrough
               }
               case service::transaction::Type::none:
               {
                  // We don't start or join any transactions
                  // (technically we join a null-trid)
                  transaction::Context::instance().join( transaction::ID{ process::handle()});
                  break;
               }
            }
         }


         message::service::transaction::State Default::transaction( bool commit)
         {
            return transaction::context().finalize( commit);
         }


         namespace local
         {
            namespace
            {
               template< typename M>
               void forward( M&& message, service::invoke::Forward&& forward)
               {
                  Trace trace{ "server::handle::policy::local::forward"};


                  if( transaction::context().pending())
                     code::raise::error( code::xatmi::service_error, "forward with pending transactions - service: ", message.service.name);

                  common::service::Lookup lookup{
                     forward.parameter.service.name,
                     decltype( common::service::lookup::Context::semantic)::no_reply};

                  // TODO make this forward work without a copy of payload...
                  auto request = message;

                  auto target = common::service::lookup::reply( std::move( lookup));

                  request.buffer = std::move( forward.parameter.payload);
                  request.service = target.service;

                  log::line( log::debug, "policy::Default::forward - request:", request);

                  communication::device::blocking::send( target.process.ipc, request);
               }

            } // <unnamed>
         } // local

         void Default::forward( service::invoke::Forward&& forward, const message::service::call::callee::Request& message)
         {
            local::forward( message, std::move( forward));
         }


         void Default::forward( service::invoke::Forward&& forward, const message::conversation::connect::callee::Request& message)
         {
            local::forward( message, std::move( forward));
         }


         void Admin::configure( server::Arguments&& arguments)
         {
            // Connection to the domain has been done before...

            if( ! arguments.resources.empty())
               code::raise::error( code::casual::invalid_semantics, "can't build and link an administration server with resources");

            policy::advertise( std::move( arguments.services));

         }

         void Admin::reply( strong::ipc::id id, message::service::call::Reply& message)
         {
            communication::device::blocking::send( id, message);
         }

         void Admin::ack( const message::service::call::ACK& message)
         {
            communication::device::blocking::send( communication::instance::outbound::service::manager::device(), message);
         }

         void Admin::statistics( strong::ipc::id id, message::event::service::Call& event)
         {
            // no-op
         }

         void Admin::transaction(
               const common::transaction::ID& trid,
               const server::Service& service)
         {
            // no-op
         }

         message::service::transaction::State Admin::transaction( bool commit)
         {
            // no-op
            return {};
         }

         void Admin::forward( common::service::invoke::Forward&& forward, const message::service::call::callee::Request& message)
         {
            code::raise::error( code::casual::invalid_semantics, "can't forward within an administration server");
         }

      } // call

   } // common::server::handle::policy
} // casual
