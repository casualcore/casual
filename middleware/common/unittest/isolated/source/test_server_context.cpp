//!
//! test_server_context.cpp
//!
//! Created on: Dec 8, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "common/server/handle.h"
#include "common/process.h"

#include "common/mockup/ipc.h"
#include "common/mockup/domain.h"

#include "common/buffer/pool.h"



// we need some values
#include <xatmi.h>


#include <fstream>


extern "C"
{

   int tpsvrinit(int argc, char **argv)
   {
      return 0;
   }

   void tpsvrdone()
   {

   }
}


namespace casual
{
   namespace common
   {
      namespace local
      {


         namespace
         {


            const std::string& replyMessage()
            {
               static const std::string reply( "reply messsage");
               return reply;
            }

            void test_service( TPSVCINFO *serviceInfo)
            {

               auto buffer = buffer::pool::Holder::instance().allocate( buffer::type::binary(), 1024);

               std::copy( replyMessage().begin(), replyMessage().end(), buffer);
               buffer[ replyMessage().size()] = '\0';

               server::Context::instance().long_jump_return( TPSUCCESS, 0, buffer, replyMessage().size(), 0);
            }

            void test_service_TPFAIL( TPSVCINFO *serviceInfo)
            {
               server::Context::instance().long_jump_return( TPFAIL, 0, serviceInfo->data, serviceInfo->len, 0);
            }

            void test_service_TPSUCCESS( TPSVCINFO *serviceInfo)
            {
               server::Context::instance().long_jump_return( TPSUCCESS, 42, serviceInfo->data, serviceInfo->len, 0);
            }

            server::Arguments arguments()
            {
               server::Arguments arguments{ { "/test/path"}};

               arguments.services.emplace_back( "test_service", &test_service, 0, server::Service::Transaction::none);

               arguments.services.emplace_back( "test_service_none_TPSUCCESS", &test_service_TPSUCCESS, 0, server::Service::Transaction::none);
               arguments.services.emplace_back( "test_service_atomic_TPSUCCESS", &test_service_TPSUCCESS, 0, server::Service::Transaction::atomic);
               arguments.services.emplace_back( "test_service_join_TPSUCCESS", &test_service_TPSUCCESS, 0, server::Service::Transaction::join);
               arguments.services.emplace_back( "test_service_auto_TPSUCCESS", &test_service_TPSUCCESS, 0, server::Service::Transaction::automatic);

               arguments.services.emplace_back( "test_service_none_TPFAIL", &test_service_TPFAIL, 0, server::Service::Transaction::none);
               arguments.services.emplace_back( "test_service_atomic_TPFAIL", &test_service_TPFAIL, 0, server::Service::Transaction::atomic);
               arguments.services.emplace_back( "test_service_join_TPFAIL", &test_service_TPFAIL, 0, server::Service::Transaction::join);
               arguments.services.emplace_back( "test_service_auto_TPFAIL", &test_service_TPFAIL, 0, server::Service::Transaction::automatic);

               return arguments;
            }


            message::service::call::callee::Request callMessage( platform::queue_id_type id)
            {
               message::service::call::callee::Request message;

               message.buffer = { buffer::type::binary(), platform::binary_type( 1024)};
               message.descriptor = 10;
               message.service.name = "test_service";
               message.process.queue = id;

               return message;
            }


         } // <unnamed>
      } // local


      TEST( casual_common_server_context, arguments)
      {
         server::Arguments arguments{ { "arg1", "arg2"}};

         ASSERT_TRUE( arguments.argc == 2);
         EXPECT_TRUE( arguments.argv[ 0] == std::string( "arg1"));
         EXPECT_TRUE( arguments.argv[ 1] == std::string( "arg2"));

         EXPECT_TRUE( arguments.arguments.at( 0) == "arg1");
         EXPECT_TRUE( arguments.arguments.at( 1) == "arg2");
      }

      TEST( casual_common_server_context, arguments_move)
      {
         server::Arguments origin{ { "arg1", "arg2"}};

         server::Arguments arguments = std::move( origin);

         ASSERT_TRUE( arguments.argc == 2);
         EXPECT_TRUE( arguments.argv[ 0] == std::string( "arg1"));
         EXPECT_TRUE( arguments.argv[ 1] == std::string( "arg2"));

         EXPECT_TRUE( arguments.arguments.at( 0) == "arg1");
         EXPECT_TRUE( arguments.arguments.at( 1) == "arg2");
      }




      TEST( casual_common_server_context, connect)
      {
         common::Trace trace{ "casual_common_server_context.connect", log::internal::debug};
         mockup::ipc::clear();

         mockup::domain::Broker broker;

         EXPECT_NO_THROW({
            server::handle::Call callHandler( local::arguments());
         });
      }



      TEST( casual_common_server_context, call_service__gives_reply)
      {
         common::Trace trace{ "casual_common_server_context.call_service__gives_reply", log::internal::debug};
         mockup::ipc::clear();

         mockup::ipc::Instance caller;
         mockup::domain::Domain domain;

         {

            server::handle::Call callHandler( local::arguments());

            auto message = local::callMessage( caller.id());
            callHandler( message);
         }


         queue::blocking::Reader reader( caller.output());
         message::service::call::Reply message;

         reader( message);
         EXPECT_TRUE( message.buffer.memory.data() == local::replyMessage());

      }


      TEST( casual_common_server_context, call_service__gives_broker_ack)
      {
         mockup::ipc::clear();
         mockup::ipc::Instance caller{ 42};

         //
         // We only need the mockup-broker during initialization.
         //
         auto prepare_caller = [](){

            mockup::domain::Broker broker;
            return server::handle::Call{ local::arguments()};
         };

         {
            auto callHandler = prepare_caller();
            auto message = local::callMessage( caller.id());
            callHandler( message);
         }

         queue::blocking::Reader broker( mockup::ipc::broker::queue().output());
         message::service::call::ACK message;

         broker( message);
         EXPECT_TRUE( message.service == "test_service");
         EXPECT_TRUE( message.process.queue == ipc::receive::id());
      }




      TEST( casual_common_server_context, call_service__gives_traffic_notify)
      {
         mockup::ipc::clear();

         mockup::ipc::Instance caller{ 10};
         mockup::ipc::Instance traffic{ 42};

         mockup::domain::Broker broker;


         {

            server::handle::Call callHandler( local::arguments());

            auto message = local::callMessage( caller.id());
            message.service.traffic_monitors = { traffic.id()};
            callHandler( message);
         }

         queue::blocking::Reader reader( traffic.output());

         message::traffic::Event message;
         reader( message);
         EXPECT_TRUE( message.service == "test_service");

      }


      namespace local
      {
         namespace
         {

            struct Domain
            {
               mockup::domain::Broker broker;
               mockup::domain::transaction::Manager tm;

            };

            namespace call
            {
               message::service::call::callee::Request request( platform::queue_id_type queue, std::string service, common::transaction::ID trid = common::transaction::ID{})
               {
                  message::service::call::callee::Request message;

                  message.correlation = uuid::make();
                  message.buffer = { buffer::type::binary(), platform::binary_type( 1024)};
                  message.descriptor = 10;
                  message.service.name = std::move( service);
                  message.process.queue = queue;
                  message.trid = std::move( trid);

                  return message;
               }

               template< typename R>
               message::service::call::Reply reply( R& receive, const Uuid& correlation)
               {
                  queue::blocking::Reader reader( receive);
                  message::service::call::Reply message;
                  reader( message, correlation);

                  return message;
               }


            } // call


            namespace transaction
            {
               const common::transaction::ID& ongoing()
               {
                  static auto singleton = common::transaction::ID::create(
                        process::Handle{ process::handle().pid + 1, process::handle().queue});

                  return singleton;
               }


            } // transaction


            namespace crete
            {
               struct handle_holder
               {
                  handle_holder() : m_handler( local::arguments()) {}

                  handle_holder(handle_holder&&) noexcept = default;
                  handle_holder& operator = (handle_holder&&) noexcept = default;

                  std::vector< mockup::reply::result_t> operator () ( message::service::call::callee::Request request)
                  {
                     m_handler( request);

                     return {};
                  }

                  server::handle::Call m_handler;
               };

               mockup::ipc::Replier server()
               {
                  return mockup::ipc::Replier{
                     mockup::reply::Handler{
                        handle_holder{}
                     }
                  };
               }
            } // crete



         } // <unnamed>
      } // local


      TEST( casual_common_server_context, connect_server)
      {
         mockup::domain::Broker broker;

         auto server = local::crete::server();
      }


      TEST( casual_common_server_context, call_server__non_existing__gives_TPESVCERR)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "non-existing-service");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCERR) << "error: " << reply.error << " - "<< common::error::xatmi::error( reply.error);
      }


      TEST( casual_common_server_context, call_server__test_service_none_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_none_TPSUCCESS");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0);
      }

      TEST( casual_common_server_context, call_server__test_service_atomic_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_atomic_TPSUCCESS");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0) << "reply.error: " << reply.error;
      }

      TEST( casual_common_server_context, call_server__test_service_join_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_join_TPSUCCESS");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0);
      }

      TEST( casual_common_server_context, call_server__test_service_auto_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_auto_TPSUCCESS");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0);
      }


      TEST( casual_common_server_context, call_server_in_transaction__test_service_none_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_none_TPSUCCESS", local::transaction::ongoing());
         server( message);

         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( casual_common_server_context, call_server_in_transaction__test_service_atomic_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_atomic_TPSUCCESS", local::transaction::ongoing());
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0) << "reply.error: " << reply.error;
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( casual_common_server_context, call_server_in_transaction__test_service_join_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_join_TPSUCCESS", local::transaction::ongoing());
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( casual_common_server_context, call_server_in_transaction__test_service_auto_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_auto_TPSUCCESS", local::transaction::ongoing());
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }


      TEST( casual_common_server_context, call_server__test_service_none_TPFAIL)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_none_TPFAIL");
         server( message);

         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL);
      }

      TEST( casual_common_server_context, call_server__test_service_atomic_TPFAIL)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_atomic_TPFAIL");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL) << "reply.error: " << reply.error;
      }

      TEST( casual_common_server_context, call_server__test_service_join_TPFAIL)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_join_TPFAIL");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL);
      }

      TEST( casual_common_server_context, call_server__test_service_auto_TPFAIL)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_auto_TPFAIL");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL);
      }


      TEST( casual_common_server_context, call_server_in_transaction__test_service_none_TPFAIL)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_none_TPFAIL", local::transaction::ongoing());
         server( message);

         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( casual_common_server_context, call_server_in_transaction__test_service_atomic_TPFAIL)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_atomic_TPFAIL", local::transaction::ongoing());
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL) << "reply.error: " << reply.error;
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( casual_common_server_context, call_server_in_transaction__test_service_join_TPFAIL__expect_rollback)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_join_TPFAIL", local::transaction::ongoing());
         server( message);

         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::rollback) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( casual_common_server_context, call_server_in_transaction__test_service_auto_TPFAIL__expect_rollback)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_auto_TPFAIL", local::transaction::ongoing());
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::rollback) << "reply.transaction.state: " << reply.transaction.state;
      }



      TEST( casual_common_server_context, state_call_descriptor_reserver)
      {
         call::State state;

         auto first = state.pending.reserve( uuid::make());
         EXPECT_TRUE( first == 1);
         auto second = state.pending.reserve( uuid::make());
         EXPECT_TRUE( second == 2);

         state.pending.unreserve( first.descriptor);
         state.pending.unreserve( second.descriptor);
      }

   } // common
} // casual



