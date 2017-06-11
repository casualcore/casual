//!
//! casual 
//!

#include <gtest/gtest.h>
#include "common/unittest.h"


#include "gateway/message.h"
#include "gateway/common.h"
#include "gateway/environment.h"


#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/environment.h"

#include "common/mockup/process.h"
#include "common/mockup/domain.h"


namespace casual
{
   using namespace common;

   namespace gateway
   {

      namespace local
      {
         namespace
         {
            struct Inbound
            {

               Inbound( platform::ipc::id::type ipc)
                 : process{ "./bin/casual-gateway-inbound-ipc",
                  {
                     "--remote-ipc-queue", std::to_string( ipc),
                     "--correlation", uuid::string( correlation),
                 }}
               {

               }

               Uuid correlation = uuid::make();
               common::mockup::Process process;
            };


            struct Domain
            {

               Domain()
                  : inbound{ communication::ipc::inbound::id()}
               {
                  try
                  {

                     //
                     // We act both as the remote outbound connection and the local gateway. Might
                     // be a little confusing, but it probably easier than to mockup all the parts.
                     //

                     //
                     // Do the connection dance...
                     //

                     message::ipc::connect::Reply reply;
                     communication::ipc::blocking::receive( communication::ipc::inbound::device(), reply, inbound.correlation);
                     external = reply.process;

                     log << "external: " << external << std::endl;

                     //
                     // act as the oubound and send discover
                     //
                     {
                        Trace trace{ "gateway::local::Domain outbound -> discover"};


                        message::interdomain::domain::discovery::receive::Request request;
                        request.process = process::handle();
                        request.domain = remote;
                        communication::ipc::blocking::send( external.queue , request);

                     }

                     //
                     // wait for the connect from the inbound
                     //
                     {
                        message::inbound::Connect connect;
                        communication::ipc::blocking::receive( communication::ipc::inbound::device(), connect);

                        EXPECT_TRUE( connect.domain == remote) << "connect: " << connect;

                     }

                  }
                  catch( ...)
                  {
                     common::error::handler();
                     throw;
                  }

               }

               common::mockup::domain::Manager manager;

               struct connect_gateway_t
               {
                  connect_gateway_t()
                  {
                     //
                     // Act as the gateway
                     //
                     process::instance::connect( process::instance::identity::gateway::manager());
                  }

               } connect_gateway;

               common::mockup::domain::service::Manager service;
               common::mockup::domain::transaction::Manager tm;
               Inbound inbound;
               process::Handle external;
               common::domain::Identity remote = common::domain::Identity{ uuid::make(), "remote-domain"};
            };


         } // <unnamed>
      } // local

      TEST( casual_gateway_inbound_ipc, shutdown_before_connection__expect_gracefull_shutdown)
      {
         common::unittest::Trace trace;

         //
         // We need to have a domain manager to 'connect the process'
         //
         common::mockup::domain::Manager manager;

         EXPECT_NO_THROW({
            local::Inbound inbound{ communication::ipc::inbound::id()};
         });

         communication::ipc::inbound::device().clear();
      }

      TEST( casual_gateway_inbound_ipc, connection_then_force_shutdown__expect_gracefull_shutdown)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            local::Domain doman;
         });

      }

      TEST( casual_gateway_inbound_ipc, connection_then_shutdown__expect_gracefull_shutdown)
      {
         common::unittest::Trace trace;

         EXPECT_THROW({
            local::Domain domain;

            communication::ipc::blocking::send(
                  domain.inbound.process.handle().queue,
                  common::message::shutdown::Request{ process::handle()});

            process::sleep( std::chrono::seconds{ 1});

         }, exception::signal::child::Terminate);
      }


      TEST( casual_gateway_inbound_ipc, service_call__service1__expect_echo)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::domain::echo::Server server{ { "service1"}};


         platform::binary::type paylaod{ 1, 2, 3, 4, 5, 6, 7, 8, 9};


         message::interdomain::service::call::receive::Request request;
         {
            request.service.name = "service1";
            request.process = process::handle();
            request.buffer = buffer::Payload{ buffer::type::binary(), paylaod};
         }


         auto reply = communication::ipc::call( domain.external.queue, request);

         EXPECT_TRUE( reply.buffer.memory == paylaod);
      }

      TEST( casual_gateway_inbound_ipc, service_call__absent_service__expect_reply_with_TPESVCERR)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         platform::binary::type paylaod{ 1, 2, 3, 4, 5, 6, 7, 8, 9};

         message::interdomain::service::call::receive::Request request;
         {
            request.service.name = "absent_service";
            request.process = process::handle();
            request.buffer = buffer::Payload{ buffer::type::binary(), paylaod};
         }

         auto reply = communication::ipc::call( domain.external.queue, request);

         EXPECT_TRUE( reply.status == TPESVCERR);
         EXPECT_TRUE( reply.buffer.memory.empty());
      }


      TEST( casual_gateway_inbound_ipc, service_call__removed_ipc_queue___expect_reply_with_TPESVCERR)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         platform::binary::type paylaod{ 1, 2, 3, 4, 5, 6, 7, 8, 9};

         message::interdomain::service::call::receive::Request request;
         {
            request.service.name = "removed_ipc_queue";
            request.process = process::handle();
            request.buffer = buffer::Payload{ buffer::type::binary(), paylaod};
         }

         auto reply = communication::ipc::call( domain.external.queue, request);

         EXPECT_TRUE( reply.status == TPESVCERR);
         EXPECT_TRUE( reply.buffer.memory.empty());
      }

   } // gateway


} // casual
