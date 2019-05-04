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
                  void start( S&& services, std::vector< argument::transaction::Resource> resources, std::function<void()> connected)
                  {
                     Trace trace{ "common::server::start"};

                     auto handler = common::communication::ipc::inbound::device().handler(
                        // will configure and advertise services
                        common::server::handle::Call( local::transform::arguments( std::move( services), std::move( resources))),
                        common::server::handle::Conversation{},
                        common::message::handle::Shutdown{},
                        common::message::handle::ping());

                     // Connect to domain
                     common::communication::instance::connect();

                     if( connected)
                        connected();

                     // Start the message-pump
                     common::message::dispatch::pump(
                           handler,
                           common::communication::ipc::inbound::device(),
                           common::communication::ipc::policy::Blocking{});

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
                  std::function<void()> connected)
            {
               local::start( std::move( services), std::move( resources), std::move( connected));
            }

         } // v1

      } // server
   } // common
} // casual
