//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "server/start.h"

#include "server/service.h"
#include "server/argument.h"
#include "server/context.h"


#include "common/message/dispatch/handle.h"
#include "common/communication/instance.h"

namespace casual
{
   namespace server
   {
      inline namespace v1
      {
         namespace local
         {
            namespace
            {
               namespace transform
               {
                  struct Service
                  {
                     server::Service operator() ( argument::Service& service)
                     {
                        return {
                           std::move( service.name),
                           std::move( service.function),
                           service.transaction,
                           service.visibility,
                           std::move( service.category),
                        };
                     }

                     server::Service operator() ( argument::xatmi::Service& service)
                     {
                        return server::xatmi::service(
                           std::move( service.name),
                           std::move( service.function),
                           service.transaction,
                           service.visibility,
                           std::move( service.category));
                     }
                  };

                  template< typename S>
                  server::Arguments arguments(
                        S services,
                        std::vector< argument::transaction::Resource> resources)
                  {
                     server::Arguments result;

                     common::algorithm::transform( services, result.services, transform::Service{});
                     result.resources = std::move( resources);

                     return result;
                  }
               } // transform

               template< typename S>
               void start( S services, std::vector< argument::transaction::Resource> resources, common::function<void()const> initialize)
               {
                  common::Trace trace{ "common::server::local::start"};
                  common::log::debug( "services: ", services);

                  auto& inbound = common::communication::ipc::inbound::device();

                  struct 
                  {
                     bool done = false;
                  } state;

                  auto handler = common::message::dispatch::handler( inbound,
                     common::message::dispatch::handle::defaults(),
                     server::Context::instance().initialize( local::transform::arguments( std::move( services), std::move( resources))),
                     [&state]( const common::message::shutdown::Request& message)
                     {
                        common::log::debug( "shutdown: ", message);
                        state.done = true;
                     });

                  auto condition = common::message::dispatch::condition::compose( 
                     common::message::dispatch::condition::prelude(
                        [initialize = std::move( initialize)]()
                        {
                           common::Trace trace{ "common::server::start prelude"};

                           if( initialize)
                              initialize();

                           // Connect to domain - send "ready"...
                           common::communication::instance::connect();
                        }),
                     common::message::dispatch::condition::done(
                           [&state]() { return state.done;}
                        )
                  );

                  // Start the message-pump
                  common::message::dispatch::pump(
                     std::move( condition),
                     handler,
                     common::communication::ipc::inbound::device());

               }

            } // <unnamed>
         } // local

         void start( std::vector< argument::Service> services, std::vector< argument::transaction::Resource> resources)
         {
            local::start( std::move( services), std::move( resources), nullptr);
         }

         void start( std::vector< argument::Service> services)
         {
            local::start( std::move( services), {}, nullptr);
         }

         void start(
            std::vector< argument::xatmi::Service> services,
            std::vector< argument::transaction::Resource> resources,
            common::function< void() const> initialize)
         {
            local::start( std::move( services), std::move( resources), std::move( initialize));
         }

      } // v1

      } // server
} // casual
