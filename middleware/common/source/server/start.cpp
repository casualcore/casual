//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/server/start.h"

#include "common/communication/instance.h"
#include "common/server/handle/call.h"
#include "common/server/handle/conversation.h"
#include "common/message/handle.h"

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
                        server::Service operator() ( argument::Service& s)
                        {
                           return {
                              std::move( s.name),
                              std::move( s.function),
                              s.transaction,
                              std::move( s.category)
                           };
                        }

                        server::Service operator() ( argument::xatmi::Service& s)
                        {
                           return server::xatmi::service(
                              std::move( s.name),
                              std::move( s.function),
                              s.transaction,
                              std::move( s.category));
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
                  void start( S&& services, std::vector< argument::transaction::Resource> resources, std::function<void()> initialize)
                  {
                     Trace trace{ "common::server::start"};

                     auto& inbound = common::communication::ipc::inbound::device();

                     auto handler = inbound.handler(
                        common::message::handle::defaults( inbound),
                        // will configure and advertise services
                        common::server::handle::Call( local::transform::arguments( std::move( services), std::move( resources))),
                        common::server::handle::Conversation{});

                     if( initialize)
                        initialize();

                     // Connect to domain - send "ready"...
                     common::communication::instance::connect();

                     // Start the message-pump
                     common::message::dispatch::pump(
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
                  std::function<void()> initialize)
            {
               local::start( std::move( services), std::move( resources), std::move( initialize));
            }

         } // v1

      } // server
   } // common
} // casual
