//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"


#include "common/server/handle.h"
#include "common/process.h"

#include "common/mockup/ipc.h"
#include "common/mockup/domain.h"
#include "common/mockup/rm.h"

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

               arguments.services.emplace_back( "test_service", &test_service, service::category::none, service::transaction::Type::none);

               arguments.services.emplace_back( "test_service_none_TPSUCCESS", &test_service_TPSUCCESS, service::category::none, service::transaction::Type::none);
               arguments.services.emplace_back( "test_service_atomic_TPSUCCESS", &test_service_TPSUCCESS, service::category::none, service::transaction::Type::atomic);
               arguments.services.emplace_back( "test_service_join_TPSUCCESS", &test_service_TPSUCCESS, service::category::none, service::transaction::Type::join);
               arguments.services.emplace_back( "test_service_auto_TPSUCCESS", &test_service_TPSUCCESS, service::category::none, service::transaction::Type::automatic);

               arguments.services.emplace_back( "test_service_none_TPFAIL", &test_service_TPFAIL, service::category::none, service::transaction::Type::none);
               arguments.services.emplace_back( "test_service_atomic_TPFAIL", &test_service_TPFAIL, service::category::none, service::transaction::Type::atomic);
               arguments.services.emplace_back( "test_service_join_TPFAIL", &test_service_TPFAIL, service::category::none, service::transaction::Type::join);
               arguments.services.emplace_back( "test_service_auto_TPFAIL", &test_service_TPFAIL, service::category::none, service::transaction::Type::automatic);

               return arguments;
            }


            message::service::call::callee::Request call_request( platform::ipc::id::type id)
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


      TEST( common_server_context, arguments)
      {
         common::unittest::Trace trace;

         server::Arguments arguments{ { "arg1", "arg2"}};

         ASSERT_TRUE( arguments.argc == 2);
         EXPECT_TRUE( arguments.argv[ 0] == std::string( "arg1"));
         EXPECT_TRUE( arguments.argv[ 1] == std::string( "arg2"));

         EXPECT_TRUE( arguments.arguments.at( 0) == "arg1");
         EXPECT_TRUE( arguments.arguments.at( 1) == "arg2");
      }

      TEST( common_server_context, arguments_move)
      {
         common::unittest::Trace trace;

         server::Arguments origin{ { "arg1", "arg2"}};

         server::Arguments arguments = std::move( origin);

         ASSERT_TRUE( arguments.argc == 2);
         EXPECT_TRUE( arguments.argv[ 0] == std::string( "arg1"));
         EXPECT_TRUE( arguments.argv[ 1] == std::string( "arg2"));

         EXPECT_TRUE( arguments.arguments.at( 0) == "arg1");
         EXPECT_TRUE( arguments.arguments.at( 1) == "arg2");
      }




      TEST( common_server_context, connect)
      {
         common::unittest::Trace trace;

         mockup::domain::Manager manager;
         mockup::domain::Broker broker;

         EXPECT_NO_THROW({
            server::handle::Call callHandler( local::arguments());
         });
      }



      TEST( common_server_context, call_service__gives_reply)
      {
         common::unittest::Trace trace;

         mockup::domain::minimal::Domain domain;
         mockup::ipc::Collector caller;

         {

            server::handle::Call callHandler( local::arguments());

            auto message = local::call_request( caller.id());
            callHandler( message);
         }

         message::service::call::Reply message;
         communication::ipc::blocking::receive( caller.output(), message);

         EXPECT_TRUE( message.buffer.memory.data() == local::replyMessage());

      }



      namespace local
      {
         namespace
         {

            struct Domain
            {
               Domain() : manager{ handle_server_configuration{}}, tm{ handle_resource_lookup{}}
               {

               }

               mockup::domain::Manager manager;
               mockup::domain::Broker broker;
               mockup::domain::transaction::Manager tm;


            private:
               struct handle_server_configuration
               {
                  void operator () ( message::domain::server::configuration::Request& request) const
                  {
                     auto reply = common::message::reverse::type( request);
                     reply.resources = { "rm1", "rm2"};

                     common::mockup::ipc::eventually::send( request.process.queue, reply);
                  }
               };

               struct handle_resource_lookup
               {
                  void operator () ( message::transaction::resource::lookup::Request& request) const
                  {
                     auto reply = common::message::reverse::type( request);
                     reply.resources = {
                           {
                              [](  message::transaction::resource::Resource& r){
                                 r.name = "rm1";
                                 r.id = 1;
                                 r.key = "rm-mockup";
                                 r.openinfo = "openinfo1";
                                 r.closeinfo = "closeinfo1";
                              }
                           },
                           {
                              [](  message::transaction::resource::Resource& r){
                                 r.name = "rm2";
                                 r.id = 2;
                                 r.key = "rm-mockup";
                                 r.openinfo = "openinfo2";
                                 r.closeinfo = "closeinfo2";
                              }
                           }
                     };

                     common::mockup::ipc::eventually::send( request.process.queue, reply);
                  }
               };
            };

            namespace call
            {
               message::service::call::callee::Request request(
                     platform::ipc::id::type queue,
                     std::string service,
                     common::transaction::ID trid = common::transaction::ID{})
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

               message::service::call::Reply reply( communication::ipc::inbound::Device& receive, const Uuid& correlation)
               {
                  message::service::call::Reply message;
                  communication::ipc::blocking::receive( receive, message, correlation);

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



         } // <unnamed>
      } // local

      TEST( common_server_context, mockup_domain_startup)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            local::Domain domain;
         });
      }

      TEST( common_server_context, resource_configuration)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto arguments = local::arguments();
         arguments.resources.emplace_back(  "rm-mockup", &casual_mockup_xa_switch_static);

         EXPECT_NO_THROW({
            server::handle::Call server( std::move( arguments));
         });
      }

      TEST( common_server_context, call_server__non_existing__gives_TPESVCERR)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "non-existing-service");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCERR) << "error: " << reply.error << " - "<< common::error::xatmi::error( reply.error);
      }



      TEST( common_server_context, call_server__test_service_none_TPSUCCESS)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_none_TPSUCCESS");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0);
      }

      TEST( common_server_context, call_server__test_service_atomic_TPSUCCESS)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_atomic_TPSUCCESS");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0) << "reply.error: " << reply.error;
      }



      TEST( common_server_context, call_server__test_service_join_TPSUCCESS)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_join_TPSUCCESS");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0);
      }

      TEST( common_server_context, call_server__test_service_auto_TPSUCCESS)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_auto_TPSUCCESS");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0);
      }




      TEST( common_server_context, call_server_in_transaction__test_service_none_TPSUCCESS)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_none_TPSUCCESS", local::transaction::ongoing());
         server( message);

         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( common_server_context, call_server_in_transaction__test_service_atomic_TPSUCCESS)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_atomic_TPSUCCESS", local::transaction::ongoing());
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0) << "reply.error: " << reply.error;
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( common_server_context, call_server_in_transaction__test_service_join_TPSUCCESS)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_join_TPSUCCESS", local::transaction::ongoing());
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( common_server_context, call_server_in_transaction__test_service_auto_TPSUCCESS)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_auto_TPSUCCESS", local::transaction::ongoing());
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == 0);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }


      TEST( common_server_context, call_server__test_service_none_TPFAIL)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_none_TPFAIL");
         server( message);

         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL);
      }

      TEST( common_server_context, call_server__test_service_atomic_TPFAIL)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_atomic_TPFAIL");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL) << "reply.error: " << reply.error;
      }

      TEST( common_server_context, call_server__test_service_join_TPFAIL)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_join_TPFAIL");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL);
      }

      TEST( common_server_context, call_server__test_service_auto_TPFAIL)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_auto_TPFAIL");
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL);
      }


      TEST( common_server_context, call_server_in_transaction__test_service_none_TPFAIL)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_none_TPFAIL", local::transaction::ongoing());
         server( message);

         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( common_server_context, call_server_in_transaction__test_service_atomic_TPFAIL)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;


         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_atomic_TPFAIL", local::transaction::ongoing());
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL) << "reply.error: " << reply.error;
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::active) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( common_server_context, call_server_in_transaction__test_service_join_TPFAIL__expect_rollback)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_join_TPFAIL", local::transaction::ongoing());
         server( message);

         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::rollback) << "reply.transaction.state: " << reply.transaction.state;
      }

      TEST( common_server_context, call_server_in_transaction__test_service_auto_TPFAIL__expect_rollback)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::ipc::Collector caller;

         server::handle::Call server( local::arguments());
         auto message = local::call::request( caller.id(), "test_service_auto_TPFAIL", local::transaction::ongoing());
         server( message);


         auto reply = local::call::reply( caller.output(), message.correlation);

         EXPECT_TRUE( reply.error == TPESVCFAIL);
         EXPECT_TRUE( reply.transaction.trid == local::transaction::ongoing());
         EXPECT_TRUE( transaction::Transaction::State( reply.transaction.state) == transaction::Transaction::State::rollback) << "reply.transaction.state: " << reply.transaction.state;
      }



      TEST( common_server_context, state_call_descriptor_reserver)
      {
         common::unittest::Trace trace;

         service::call::State state;

         auto first = state.pending.reserve( uuid::make());
         EXPECT_TRUE( first == 1);
         auto second = state.pending.reserve( uuid::make());
         EXPECT_TRUE( second == 2);

         state.pending.unreserve( first.descriptor);
         state.pending.unreserve( second.descriptor);
      }


   } // common
} // casual



