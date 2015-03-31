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

               server::Context::instance().longJumpReturn( TPSUCCESS, 0, buffer, replyMessage().size(), 0);
            }

            void test_service_TPFAIL( TPSVCINFO *serviceInfo)
            {
               server::Context::instance().longJumpReturn( TPFAIL, 0, serviceInfo->data, serviceInfo->len, 0);
            }

            void test_service_TPSUCCESS( TPSVCINFO *serviceInfo)
            {
               server::Context::instance().longJumpReturn( TPSUCCESS, 42, serviceInfo->data, serviceInfo->len, 0);
            }

            server::Arguments arguments()
            {
               server::Arguments arguments{ { "/test/path"}};

               arguments.services.emplace_back( "test_service", &test_service, 0, server::Service::cNone);

               arguments.services.emplace_back( "test_service_none_TPSUCCESS", &test_service_TPSUCCESS, 0, server::Service::cNone);
               arguments.services.emplace_back( "test_service_atomic_TPSUCCESS", &test_service_TPSUCCESS, 0, server::Service::cAtomic);
               arguments.services.emplace_back( "test_service_join_TPSUCCESS", &test_service_TPSUCCESS, 0, server::Service::cJoin);
               arguments.services.emplace_back( "test_service_auto_TPSUCCESS", &test_service_TPSUCCESS, 0, server::Service::cAuto);

               arguments.services.emplace_back( "test_service_none_TPFAIL", &test_service_TPFAIL, 0, server::Service::cNone);
               arguments.services.emplace_back( "test_service_atomic_TPFAIL", &test_service_TPFAIL, 0, server::Service::cAtomic);
               arguments.services.emplace_back( "test_service_join_TPFAIL", &test_service_TPFAIL, 0, server::Service::cJoin);
               arguments.services.emplace_back( "test_service_auto_TPFAIL", &test_service_TPFAIL, 0, server::Service::cAuto);

               return arguments;
            }


            message::service::call::callee::Request callMessage( platform::queue_id_type id)
            {
               message::service::call::callee::Request message;

               message.buffer = { buffer::type::binary(), platform::binary_type( 1024)};
               message.descriptor = 10;
               message.service.name = "test_service";
               message.reply.queue = id;

               return message;
            }



            namespace broker
            {
               void prepare( platform::queue_id_type id)
               {
                  common::queue::blocking::Writer send( id);
                  // server connect
                  {
                     message::server::connect::Reply reply;
                     send( reply);
                  }

                  // transaction client connect
                  {
                     message::transaction::client::connect::Reply reply;
                     reply.domain = "unittest-domain";
                     reply.transactionManagerQueue = mockup::ipc::transaction::manager::queue().id();
                     send( reply);
                  }

               }
            } // broker

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
         mockup::ipc::clear();

         mockup::ipc::Router router{ ipc::receive::id()};

         //
         // Prepare "broker response"
         //
         {
            local::broker::prepare( router.id());
         }

         server::handle::Call callHandler( local::arguments());

         message::server::connect::Request message;

         queue::blocking::Reader read( mockup::ipc::broker::queue().receive());
         read( message);

         EXPECT_TRUE( message.process == process::handle());

         ASSERT_TRUE( message.services.size() > 1);
         EXPECT_TRUE( message.services.at( 0).name == "test_service");

      }



      TEST( casual_common_server_context, call_service__gives_reply)
      {
         mockup::ipc::clear();

         // just a cache to keep queue writable
         mockup::ipc::Router router{ ipc::receive::id()};

         // instance that "calls" a service in callee::handle::Call, and get's a reply
         mockup::ipc::Instance caller( 10);

         {
            local::broker::prepare( router.id());
            server::handle::Call callHandler( local::arguments());

            auto message = local::callMessage( caller.id());
            callHandler( message);
         }


         queue::blocking::Reader reader( caller.receive());
         message::service::call::Reply message;

         reader( message);
         EXPECT_TRUE( message.buffer.memory.data() == local::replyMessage());

      }


      TEST( casual_common_server_context, call_service__gives_broker_ack)
      {
         mockup::ipc::clear();

         // just a cache to keep queue writable
         mockup::ipc::Router router{ ipc::receive::id()};

         // instance that "calls" a service in callee::handle::Call, and get's a reply
         mockup::ipc::Instance caller( 10);

         {
            local::broker::prepare( router.id());
            server::handle::Call callHandler( local::arguments());

            auto message = local::callMessage( caller.id());
            callHandler( message);
         }

         queue::blocking::Reader broker( mockup::ipc::broker::queue().receive());
         message::service::call::ACK message;

         broker( message);
         EXPECT_TRUE( message.service == "test_service");
         EXPECT_TRUE( message.process.queue == ipc::receive::id());
      }


      TEST( casual_common_server_context, call_non_existing_service__throws)
      {
         mockup::ipc::clear();

         // just a cache to keep queue writable
         mockup::ipc::Router router{ ipc::receive::id()};

         // instance that "calls" a service in callee::handle::Call, and get's a reply
         mockup::ipc::Instance caller( 10);

         local::broker::prepare( router.id());
         server::handle::Call callHandler( local::arguments());

         auto message = local::callMessage( caller.id());
         message.service.name = "non_existing";


         EXPECT_THROW( {
            callHandler( message);
         }, exception::xatmi::SystemError);
      }



      TEST( casual_common_server_context, call_service__gives_monitor_notify)
      {
         mockup::ipc::clear();

         // just a cache to keep queue writable
         mockup::ipc::Router router{ ipc::receive::id()};

         // instance that "calls" a service in callee::handle::Call, and get's a reply
         mockup::ipc::Instance caller( 10);

         mockup::ipc::Instance monitor( 42);

         {
            local::broker::prepare( router.id());

            server::handle::Call callHandler( local::arguments());

            auto message = local::callMessage( caller.id());
            message.service.monitor_queue = monitor.id();
            callHandler( message);
         }

         queue::blocking::Reader reader( monitor.receive());

         message::monitor::Notify message;
         reader( message);
         EXPECT_TRUE( message.service == "test_service");

         mockup::ipc::broker::queue().clear();
         ipc::receive::queue().clear();
      }


      namespace local
      {
         namespace
         {

            struct Domain
            {
               //
               // Set up a 'domain'
               //
               Domain()
                  :
                  broker{ ipc::receive::id(), mockup::create::broker()},
                  // link the global mockup-broker-queue's output to 'our' broker
                  link_broker_reply{ mockup::ipc::broker::queue().receive().id(), broker.id()},

                  tm{ ipc::receive::id(), mockup::create::transaction::manager()},
                  // link the global mockup-transaction-manager-queue's output to 'our' tm
                  link_tm_reply{ mockup::ipc::transaction::manager::queue().receive().id(), tm.id()}
               {

               }

               mockup::ipc::Router broker;
               mockup::ipc::Link link_broker_reply;
               mockup::ipc::Router tm;
               mockup::ipc::Link link_tm_reply;

            };

            namespace call
            {
               message::service::call::callee::Request request( platform::queue_id_type queue, std::string service, common::transaction::ID trid = common::transaction::ID{})
               {
                  message::service::call::callee::Request message;

                  message.buffer = { buffer::type::binary(), platform::binary_type( 1024)};
                  message.descriptor = 10;
                  message.service.name = std::move( service);
                  message.reply.queue = queue;
                  message.trid = std::move( trid);

                  return message;
               }

               template< typename R>
               message::service::call::Reply reply( R& receive)
               {
                  queue::blocking::Reader reader( receive);
                  message::service::call::Reply message;
                  reader( message);

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


            /*
               arguments.services.emplace_back( "test_service_none_TPSUCCESS", &test_service_TPSUCCESS, 0, server::Service::cNone);
               arguments.services.emplace_back( "test_service_atomic_TPSUCCESS", &test_service_TPSUCCESS, 0, server::Service::cAtomic);
               arguments.services.emplace_back( "test_service_join_TPSUCCESS", &test_service_TPSUCCESS, 0, server::Service::cJoin);
               arguments.services.emplace_back( "test_service_auto_TPSUCCESS", &test_service_TPSUCCESS, 0, server::Service::cAuto);

               arguments.services.emplace_back( "test_service_none_TPFAIL", &test_service_TPFAIL, 0, server::Service::cNone);
               arguments.services.emplace_back( "test_service_atomic_TPFAIL", &test_service_TPFAIL, 0, server::Service::cAtomic);
               arguments.services.emplace_back( "test_service_join_TPFAIL", &test_service_TPFAIL, 0, server::Service::cJoin);
               arguments.services.emplace_back( "test_service_auto_TPFAIL", &test_service_TPFAIL, 0, server::Service::cAuto);
             */



         } // <unnamed>
      } // local


      TEST( casual_common_server_context, connect_server)
      {
         local::Domain domain;

         server::handle::Call server( local::arguments());
      }


      TEST( casual_common_server_context, call_server__test_service_none_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_none_TPSUCCESS");
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == 0);
      }

      TEST( casual_common_server_context, call_server__test_service_atomic_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_atomic_TPSUCCESS");
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == 0) << "reply.error: " << reply.error;
      }

      TEST( casual_common_server_context, call_server__test_service_join_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_join_TPSUCCESS");
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == 0);
      }

      TEST( casual_common_server_context, call_server__test_service_auto_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_auto_TPSUCCESS");
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == 0);
      }


      TEST( casual_common_server_context, call_server_in_transaction__test_service_none_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_none_TPSUCCESS", local::transaction::ongoing());
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == 0);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( casual_common_server_context, call_server_in_transaction__test_service_atomic_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_atomic_TPSUCCESS", local::transaction::ongoing());
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == 0) << "reply.error: " << reply.error;
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( casual_common_server_context, call_server_in_transaction__test_service_join_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_join_TPSUCCESS", local::transaction::ongoing());
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == 0);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( casual_common_server_context, call_server_in_transaction__test_service_auto_TPSUCCESS)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_auto_TPSUCCESS", local::transaction::ongoing());
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == 0);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }


      TEST( casual_common_server_context, call_server__test_service_none_TPFAIL)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_none_TPFAIL");
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == TPESVCFAIL);
      }

      TEST( casual_common_server_context, call_server__test_service_atomic_TPFAIL)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_atomic_TPFAIL");
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == TPESVCFAIL) << "reply.error: " << reply.error;
      }

      TEST( casual_common_server_context, call_server__test_service_join_TPFAIL)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_join_TPFAIL");
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == TPESVCFAIL);
      }

      TEST( casual_common_server_context, call_server__test_service_auto_TPFAIL)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_auto_TPFAIL");
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == TPESVCFAIL);
      }


      TEST( casual_common_server_context, call_server_in_transaction__test_service_none_TPFAIL)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_none_TPFAIL", local::transaction::ongoing());
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == TPESVCFAIL);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( casual_common_server_context, call_server_in_transaction__test_service_atomic_TPFAIL)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_atomic_TPFAIL", local::transaction::ongoing());
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == TPESVCFAIL) << "reply.error: " << reply.error;
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( casual_common_server_context, call_server_in_transaction__test_service_join_TPFAIL__expect_rollback)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_join_TPFAIL", local::transaction::ongoing());
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

         EXPECT_TRUE( reply.error == TPESVCFAIL);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::rollback) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( casual_common_server_context, call_server_in_transaction__test_service_auto_TPFAIL__expect_rollback)
      {
         local::Domain domain;

         mockup::ipc::Instance caller{ 10};

         {
            server::handle::Call server( local::arguments());
            auto message = local::call::request( caller.id(), "test_service_auto_TPFAIL", local::transaction::ongoing());
            server( message);
         }

         auto reply = local::call::reply( caller.receive());

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

         state.pending.unreserve( first);
         state.pending.unreserve( second);
      }

   } // common
} // casual



