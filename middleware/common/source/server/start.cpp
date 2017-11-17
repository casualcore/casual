//!
//! casual 
//!

#include "common/server/start.h"

#include "common/communication/ipc.h"
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

                        algorithm::transform( resources, result.resources, [](argument::transaction::Resource& r){
                           return transaction::Resource{
                              std::move( r.key),
                              r.xa_switch
                           };
                        });

                        return result;
                     }
                  } // transform

                  template< typename S>
                  void start( S&& services, std::vector< argument::transaction::Resource> resources, std::function<void()> connected)
                  {
                     Trace trace{ "common::server::start"};


                     //
                     // Connect to domain
                     //
                     common::process::instance::connect();

                     auto handler = common::communication::ipc::inbound::device().handler(
                        common::server::handle::Call( local::transform::arguments( std::move( services), std::move( resources))),
                        common::server::handle::Conversation{},
                        common::message::handle::Shutdown{},
                        common::message::handle::ping());

                     if( connected)
                        connected();


                     //
                     // Start the message-pump
                     //
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


            int main( int argc, char **argv, std::function< void( int, char**)> local_main)
            {
               try
               {
                  local_main( argc, argv);
                  return 0;
               }
               catch( ...)
               {
                  return common::exception::handle();
               }
            }

         } // v1

      } // server
   } // common
} // casual
