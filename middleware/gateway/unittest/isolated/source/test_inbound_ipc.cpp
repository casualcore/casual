//!
//! casual 
//!

#include <gtest/gtest.h>


#include "gateway/message.h"

#include "common/message/dispatch.h"
#include "common/message/handle.h"

#include "common/mockup/process.h"
#include "common/mockup/domain.h"


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
            struct Inbound
            {

               Inbound( platform::queue_id_type ipc)
                 : process{ "./bin/casual-gateway-inbound-ipc", {
                        "-ipc", std::to_string( ipc)}}
               {

               }

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
                     // Do the connection dance...
                     //

                     message::ipc::connect::Reply reply;
                     communication::ipc::blocking::receive( communication::ipc::inbound::device(), reply);
                     external = reply.process;

                     log::internal::gateway << "external: " << external << std::endl;


                     common::message::dispatch::Handler handler{
                        common::message::handle::ping()
                     };

                     handler( communication::ipc::inbound::device().next( communication::ipc::policy::Blocking{}));
                  }
                  catch( ...)
                  {
                     common::error::handler();
                     throw;
                  }

               }

               common::mockup::domain::Domain domain;
               Inbound inbound;
               process::Handle external;
            };

         } // <unnamed>
      } // local

      TEST( casual_gateway_inbound, shutdown_before_connection__expect_gracefull_shutdown)
      {
         Trace trace{ "casual_gateway_inbound shutdown_before_connection__expect_gracefull_shutdown"};

         //
         // We need to have a broker to 'connect the process'
         //
         common::mockup::domain::Broker broker;

         EXPECT_NO_THROW({
            local::Inbound inbound{ communication::ipc::inbound::id()};
         });

         communication::ipc::inbound::device().clear();
      }

      TEST( casual_gateway_inbound, connection_then_force_shutdown__expect_gracefull_shutdown)
      {
         Trace trace{ "casual_gateway_inbound connection_then_force_shutdown__expect_gracefull_shutdown"};

         EXPECT_NO_THROW({
            local::Domain doman;
         });

      }

      TEST( casual_gateway_inbound, connection_then_shutdown__expect_gracefull_shutdown)
      {
         Trace trace{ "casual_gateway_inbound connection_then_shutdown__expect_gracefull_shutdown"};

         EXPECT_THROW({
            local::Domain domain;

            communication::ipc::blocking::send(
                  domain.inbound.process.handle().queue,
                  common::message::shutdown::Request{ process::handle()});

            process::sleep( std::chrono::seconds{ 1});

         }, exception::signal::child::Terminate);
      }


      TEST( casual_gateway_inbound, service_call__service1__expect_echo)
      {
         Trace trace{ "TEST( casual_gateway_inbound, service_call__service1__expect_echo)"};

         local::Domain domain;

         platform::binary_type paylaod{ 1, 2, 3, 4, 5, 6, 7, 8, 9};

         common::message::service::call::callee::Request request;
         {
            request.service.name = "service1";
            request.process = process::handle();
            request.buffer = buffer::Payload{ buffer::type::binary(), paylaod};
         }

         auto reply = communication::ipc::call( domain.external.queue, request);

         EXPECT_TRUE( reply.buffer.memory == paylaod);
      }

      TEST( casual_gateway_inbound, service_call__absent_service__expect_reply_with_TPESVCERR)
      {
         Trace trace{ "TEST( casual_gateway_inbound, service_call__absent_service__expect_reply_with_TPESVCERR)"};

         local::Domain domain;

         platform::binary_type paylaod{ 1, 2, 3, 4, 5, 6, 7, 8, 9};

         common::message::service::call::callee::Request request;
         {
            request.service.name = "absent_service";
            request.process = process::handle();
            request.buffer = buffer::Payload{ buffer::type::binary(), paylaod};
         }

         auto reply = communication::ipc::call( domain.external.queue, request);

         EXPECT_TRUE( reply.error == TPESVCERR);
         EXPECT_TRUE( reply.buffer.memory.empty());
      }


      TEST( casual_gateway_inbound, service_call__removed_ipc_queue___expect_reply_with_TPESVCERR)
      {
         Trace trace{ "TEST( casual_gateway_inbound, service_call__removed_ipc_queue___expect_reply_with_TPESVCERR)"};

         local::Domain domain;

         platform::binary_type paylaod{ 1, 2, 3, 4, 5, 6, 7, 8, 9};

         common::message::service::call::callee::Request request;
         {
            request.service.name = "removed_ipc_queue";
            request.process = process::handle();
            request.buffer = buffer::Payload{ buffer::type::binary(), paylaod};
         }

         auto reply = communication::ipc::call( domain.external.queue, request);

         EXPECT_TRUE( reply.error == TPESVCERR);
         EXPECT_TRUE( reply.buffer.memory.empty());
      }

   } // gateway


} // casual
