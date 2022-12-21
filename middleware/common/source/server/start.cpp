//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/server/start.h"

#include "common/communication/instance.h"
#include "common/server/handle/call.h"
#include "common/server/handle/conversation.h"
#include "common/message/dispatch/handle.h"

namespace casual
{
   namespace common
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
                     common::server::Arguments arguments(
                           S&& services,
                           std::vector< argument::transaction::Resource> resources)
                     {
                        common::server::Arguments result;

                        algorithm::transform( services, result.services, transform::Service{});
                        result.resources = std::move( resources);

                        return result;
                     }
                  } // transform

                  template< typename S>
                  void start( S&& services, std::vector< argument::transaction::Resource> resources, common::function<void()const> initialize)
                  {
                     Trace trace{ "common::server::start"};
                     log::line( verbose::log, "services: ", services);

                     auto& inbound = communication::ipc::inbound::device();

                     struct 
                     {
                        bool done = false;
                     } state;

                     auto handler = message::dispatch::handler( inbound,
                        message::dispatch::handle::defaults(),
                        // will configure and advertise services
                        server::handle::Call( local::transform::arguments( std::move( services), std::move( resources))),
                        server::handle::Conversation{},
                        [&state]( const common::message::shutdown::Request& message)
                        {
                           log::line( verbose::log, "shutdown: ", message);
                           state.done = true;
                        });

                     auto condition = message::dispatch::condition::compose( 
                        message::dispatch::condition::prelude(
                           [initialize = std::move( initialize)]()
                           {
                              Trace trace{ "common::server::start prelude"};

                              if( initialize)
                                 initialize();

                              // Connect to domain - send "ready"...
                              communication::instance::connect();
                           }),
                        message::dispatch::condition::done(
                              [&state]() { return state.done;}
                           )
                     );

                     // Start the message-pump
                     message::dispatch::pump(
                        std::move( condition),
                        handler,
                        communication::ipc::inbound::device());

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
   } // common
} // casual
