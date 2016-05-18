//!
//! casual 
//!

#include <gtest/gtest.h>
#include "common/unittest.h"


#include "gateway/message.h"
#include "gateway/environment.h"

#include "common/message/dispatch.h"
#include "common/message/handle.h"

#include "common/mockup/process.h"
#include "common/mockup/domain.h"
#include "common/mockup/file.h"


#include "common/trace.h"


namespace casual
{
   using namespace common;

   namespace gateway
   {

      namespace local
      {
         namespace
         {
            struct Outbound
            {

               Outbound( const std::string& path)
                 : process{ "./bin/casual-gateway-outbound-ipc",
                     { "--domain-file", path}}
               {

               }

               common::mockup::Process process;
            };


            file::scoped::Path create_domain_file( platform::ipc::id::type queue)
            {
               Trace trace{ "create_domain_file"};

               auto file = mockup::file::temporary( "", std::to_string( queue));

               log::internal::debug << "created domain file: " << file << " - qid: " << queue << '\n';

               return file;
            }

            struct Domain
            {

               Domain()
                  : path{ create_domain_file( manager.process().queue)},
                     outbound{ path}
               {
                  try
                  {

                     //
                     // Wait for the outbound to connect (which it think we are it's remote gateway)
                     //
                     {
                        Trace trace{ "wait for connect from outbound"};

                        message::ipc::connect::Request request;
                        communication::ipc::blocking::receive( communication::ipc::inbound::device(), request);

                        //
                        // save the external entry point
                        //
                        external = request.process;
                        auto reply = common::message::reverse::type( request);

                        //
                        // remote instance act as the inbound gateway (that pairs up with the outbound)
                        //
                        reply.process = remote.process();
                        communication::ipc::blocking::send( external.queue, reply);
                     }

                  }
                  catch( ...)
                  {
                     common::error::handler();
                     throw;
                  }

               }


               common::mockup::domain::Manager manager;

               file::scoped::Path path;


               struct connect_gateway_t
               {
                  connect_gateway_t()
                  {
                     Trace trace{ "register remote gateway"};

                     //
                     // We'll act as the remote gateway our self
                     //
                     process::instance::connect( process::instance::identity::gateway::manager());
                  }

               } connect_gateway;

               common::mockup::domain::Broker broker;
               common::mockup::domain::transaction::Manager tm;
               common::mockup::ipc::Collector remote;
               Outbound outbound;
               process::Handle external;
            };

         } // <unnamed>
      } // local

      TEST( casual_gateway_outbound_ipc, shutdown_before_connection__expect_gracefull_shutdown)
      {
         CASUAL_UNITTEST_TRACE();

         //
         // We need to have a domain manager to 'connect the process'
         //
         common::mockup::domain::Manager manager;
         auto path = local::create_domain_file( communication::ipc::inbound::id());

         EXPECT_NO_THROW({
            local::Outbound outbound{ path};
         });

         communication::ipc::inbound::device().clear();
      }

      TEST( casual_gateway_outbound_ipc, connection_then_force_shutdown__expect_gracefull_shutdown)
      {
         CASUAL_UNITTEST_TRACE();

         EXPECT_NO_THROW({
            local::Domain doman;
         });
      }

      TEST( casual_gateway_outbound_ipc, connection_then_shutdown__expect_gracefull_shutdown)
      {
         CASUAL_UNITTEST_TRACE();

         EXPECT_THROW({
            local::Domain domain;

            communication::ipc::blocking::send(
                  domain.outbound.process.handle().queue,
                  common::message::shutdown::Request{ process::handle()});

            process::sleep( std::chrono::seconds{ 1});

         }, exception::signal::child::Terminate);
      }

      TEST( casual_gateway_outbound_ipc, service_call__expect_echo)
      {
         CASUAL_UNITTEST_TRACE();

         local::Domain domain;

         platform::binary_type paylaod{ 1, 2, 3, 4, 5, 6, 7, 8, 9};

         common::message::service::call::callee::Request request;
         {
            request.service.name = "service1";
            request.process = process::handle();
            request.buffer = buffer::Payload{ buffer::type::binary(), paylaod};
         }

         auto correlation = communication::ipc::blocking::send( domain.outbound.process.handle().queue, request);

         //
         // handle the message, as we're the other domain
         //
         {
            common::message::service::call::callee::Request message;
            communication::ipc::blocking::receive( domain.remote.output(), message);

            EXPECT_TRUE( message.correlation == correlation);
            //EXPECT_TRUE( message.process == domain.external) << "message.process: "<< message.process << ", domain.external: " << domain.external;

            //
            // Reply to the outbound, since we act as an remote inbound gateway we have to set the destination our self
            // (use domain.external which we know is the entry point for replies).
            //
            auto reply = common::message::reverse::type( message);
            reply.buffer = std::move( message.buffer);
            communication::ipc::blocking::send( domain.external.queue, reply);

         }

         //
         // Get the reply from the outbound
         //
         {
            common::message::service::call::Reply reply;
            communication::ipc::blocking::receive( communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.correlation == correlation);
            EXPECT_TRUE( reply.buffer.memory == paylaod);
         }
      }


   } // gateway


} // casual

