//!
//! casual 
//!

#include "common/unittest.h"
#include "common/marshal/network.h"
#include "common/marshal/complete.h"
#include "common/communication/device.h"
#include "common/communication/ipc.h"
#include "common/mockup/ipc.h"

#include "gateway/message.h"


namespace casual
{
   namespace gateway
   {
      namespace message
      {
         namespace local
         {
            namespace
            {
               namespace general
               {
                  template< typename M>
                  void intitialize( M& message)
                  {
                     //message.process = common::process::handle();
                     message.correlation = common::uuid::make();
                     message.execution = common::uuid::make();
                  }
               } // general

               namespace transaction
               {
                  template <typename S, typename R, common::message::Type Interdomain_type>
                  struct Policy
                  {
                     constexpr common::message::Type interdomain_type() const { return Interdomain_type;}

                     S create_send()
                     {
                        S message;
                        general::intitialize( message);
                        message.resource = -10;
                        message.trid = common::transaction::ID::create();
                        return message;
                     }

                     R create_receive()
                     {
                        return {};
                     }

                     void compare( const S& send_message, const R& receive_message)
                     {
                        EXPECT_TRUE( send_message.correlation == receive_message.correlation);
                        EXPECT_TRUE( send_message.execution == receive_message.execution);
                        EXPECT_TRUE( send_message.resource == receive_message.resource);
                        EXPECT_TRUE( send_message.trid == receive_message.trid);
                     }
                  };

               } // transaction

               namespace call
               {
                  namespace request
                  {
                     struct Policy
                     {
                        constexpr common::message::Type interdomain_type() const { return common::message::Type::interdomain_service_call;}

                        common::message::service::call::callee::Request create_send()
                        {
                           common::message::service::call::callee::Request message;
                           general::intitialize( message);
                           message.flags = common::message::service::call::request::Flag::no_transaction;
                           message.header = { { "key", "value"}};
                           message.trid = common::transaction::ID::create();
                           message.parent = "parent_service";
                           message.service.name = "service-name";
                           message.service.timeout = std::chrono::microseconds{ 2000};
                           message.service.category = 42;

                           message.buffer.type = "bufer/type";

                           message.buffer.memory = common::unittest::random::binary( 4096);

                           return message;
                        }

                        message::interdomain::service::call::receive::Request create_receive()
                        {
                           return {};
                        }

                        void compare( const common::message::service::call::callee::Request& send_message,
                              const message::interdomain::service::call::receive::Request& receive_message)
                        {
                           EXPECT_TRUE( send_message.correlation == receive_message.correlation);
                           EXPECT_TRUE( send_message.execution == receive_message.execution);
                           EXPECT_TRUE( send_message.flags == receive_message.flags);
                           //EXPECT_TRUE( send_message.header == receive_message.header);
                           EXPECT_TRUE( send_message.trid == receive_message.trid);
                           EXPECT_TRUE( send_message.parent == receive_message.parent);
                           EXPECT_TRUE( send_message.service.name == receive_message.service.name);
                           //EXPECT_TRUE( send_message.service.timeout == receive_message.service.timeout);
                           EXPECT_TRUE( send_message.service.transaction == receive_message.service.transaction);
                           //EXPECT_TRUE( send_message.service.type == receive_message.service.type);

                           EXPECT_TRUE( send_message.buffer.type == receive_message.buffer.type);

                           EXPECT_TRUE( send_message.buffer.memory == receive_message.buffer.memory);

                        }
                     };
                  } // request

                  namespace reply
                  {
                     struct Policy
                     {
                        constexpr common::message::Type interdomain_type() const { return common::message::Type::interdomain_service_reply;}

                        common::message::service::call::Reply create_send()
                        {
                           common::message::service::call::Reply message;
                           general::intitialize( message);
                           message.code = 43;
                           message.error = 666;
                           message.transaction.trid = common::transaction::ID::create();
                           message.transaction.state = 777;

                           message.buffer.type = "buffer/name";

                           message.buffer.memory = common::unittest::random::binary( 4096);

                           return message;
                        }

                        message::interdomain::service::call::receive::Reply create_receive()
                        {
                           return {};
                        }

                        void compare( const common::message::service::call::Reply& send_message,
                              const message::interdomain::service::call::receive::Reply& receive_message)
                        {
                           EXPECT_TRUE( send_message.correlation == receive_message.correlation);
                           EXPECT_TRUE( send_message.execution == receive_message.execution);
                           EXPECT_TRUE( send_message.code == receive_message.code);
                           EXPECT_TRUE( send_message.error == receive_message.error);
                           EXPECT_TRUE( send_message.transaction.trid == receive_message.transaction.trid);
                           EXPECT_TRUE( send_message.transaction.state == receive_message.transaction.state);

                           EXPECT_TRUE( send_message.buffer.type == receive_message.buffer.type);

                           EXPECT_TRUE( send_message.buffer.memory == receive_message.buffer.memory);

                        }
                     };

                  } // reply
               } // call

               namespace enqueue
               {
                  namespace request
                  {
                     struct Policy
                     {
                        constexpr common::message::Type interdomain_type() const { return common::message::Type::interdomain_queue_enqueue_request;}

                        using send_type = common::message::queue::enqueue::Request;
                        using receive_type = message::interdomain::queue::enqueue::receive::Request;

                        send_type create_send()
                        {
                           send_type message;
                           general::intitialize( message);
                           message.name = "queue1";
                           message.message.payload = common::unittest::random::binary( 1024);
                           message.message.properties = "bla bla";
                           message.message.reply = "blo blo";
                           message.trid = common::transaction::ID::create();


                           return message;
                        }

                        receive_type create_receive()
                        {
                           return {};
                        }

                        void compare( const send_type& send_message, const receive_type& receive_message)
                        {
                           EXPECT_TRUE( send_message.correlation == receive_message.correlation);
                           EXPECT_TRUE( send_message.execution == receive_message.execution);
                           EXPECT_TRUE( send_message.name == receive_message.name);
                           EXPECT_TRUE( send_message.message.payload == receive_message.message.payload);
                        }
                     };
                  } // request


                  namespace reply
                  {
                     struct Policy
                     {
                        constexpr common::message::Type interdomain_type() const { return common::message::Type::interdomain_queue_enqueue_reply;}

                        using send_type = common::message::queue::enqueue::Reply;
                        using receive_type = message::interdomain::queue::enqueue::receive::Reply;

                        send_type create_send()
                        {
                           send_type message;
                           general::intitialize( message);
                           message.id = common::uuid::make();

                           return message;
                        }

                        receive_type create_receive()
                        {
                           return {};
                        }

                        void compare( const send_type& send_message, const receive_type& receive_message)
                        {
                           EXPECT_TRUE( send_message.correlation == receive_message.correlation);
                           EXPECT_TRUE( send_message.execution == receive_message.execution);
                           EXPECT_TRUE( send_message.id == receive_message.id);
                        }
                     };
                  } // reply

               } // enqueue

               namespace discovery
               {
                  namespace request
                  {
                     struct Policy
                     {
                        constexpr common::message::Type interdomain_type() const { return common::message::Type::interdomain_domain_discover_request;}

                        common::message::gateway::domain::discover::Request create_send()
                        {
                           common::message::gateway::domain::discover::Request message;
                           general::intitialize( message);
                           message.domain = common::domain::identity();
                           message.queues.emplace_back( "queue1");


                           return message;
                        }

                        message::interdomain::domain::discovery::receive::Request create_receive()
                        {
                           return {};
                        }

                        void compare( const common::message::gateway::domain::discover::Request& send_message,
                              const message::interdomain::domain::discovery::receive::Request& receive_message)
                        {
                           EXPECT_TRUE( send_message.correlation == receive_message.correlation);
                           EXPECT_TRUE( send_message.execution == receive_message.execution);
                           EXPECT_TRUE( send_message.domain == receive_message.domain);
                           EXPECT_TRUE( send_message.queues == receive_message.queues);

                        }
                     };

                  } // request


               } // discovery

            } // <unnamed>
         } // local


         template <typename H>
         struct gateway_message_interdomain : public ::testing::Test, public H
         {
         };


         typedef ::testing::Types<
               local::transaction::Policy<
                  common::message::transaction::resource::prepare::Request,
                  message::interdomain::transaction::resource::receive::prepare::Request,
                  common::message::Type::interdomain_transaction_resource_prepare_request>,
               local::transaction::Policy<
                  common::message::transaction::resource::prepare::Reply,
                  message::interdomain::transaction::resource::receive::prepare::Reply,
                  common::message::Type::interdomain_transaction_resource_prepare_reply>,
               local::transaction::Policy<
                  common::message::transaction::resource::rollback::Request,
                  message::interdomain::transaction::resource::receive::rollback::Request,
                  common::message::Type::interdomain_transaction_resource_rollback_request>,
               local::transaction::Policy<
                  common::message::transaction::resource::rollback::Reply,
                  message::interdomain::transaction::resource::receive::rollback::Reply,
                  common::message::Type::interdomain_transaction_resource_rollback_reply>,
               local::transaction::Policy<
                  common::message::transaction::resource::commit::Request,
                  message::interdomain::transaction::resource::receive::commit::Request,
                  common::message::Type::interdomain_transaction_resource_commit_request>,
               local::transaction::Policy<
                  common::message::transaction::resource::commit::Reply,
                  message::interdomain::transaction::resource::receive::commit::Reply,
                  common::message::Type::interdomain_transaction_resource_commit_reply>,
               local::call::request::Policy,
               local::call::reply::Policy,
               local::discovery::request::Policy,
               local::enqueue::request::Policy,
               local::enqueue::reply::Policy
          > message_types;

         TYPED_TEST_CASE(gateway_message_interdomain, message_types);

         TYPED_TEST( gateway_message_interdomain, network_serialization)
         {
            common::unittest::Trace trace;
            auto send_message = TestFixture::create_send();

            auto wrap = []( decltype( TestFixture::create_send())& message)
            {
               auto&& wrapped = message::interdomain::send::wrap( message);

               return common::marshal::complete( wrapped, common::marshal::binary::network::create::Output{});
            };

            auto complete = wrap( send_message);

            // check that we got the mapped typed for interdomain
            // TODO: EXPECT_TRUE( complete.category == TestFixture::interdomain_type());

            auto receive_message = TestFixture::create_receive();

            common::marshal::complete( complete, receive_message, common::marshal::binary::network::create::Input{});

            TestFixture::compare( send_message, receive_message);
         }


         TYPED_TEST( gateway_message_interdomain, network_serialization_over_ipc)
         {
            common::unittest::Trace trace;

            using inbound_type = common::communication::inbound::Device<
                  common::communication::ipc::inbound::Connector, common::marshal::binary::network::create::Input>;

            inbound_type inbound;

            auto send_message = TestFixture::create_send();

            {
               auto&& wrapped = message::interdomain::send::wrap( send_message);

               //
               // We serialize to network representation and send it over ipc
               //
               common::mockup::ipc::eventually::send(
                     inbound.connector().id(),
                     wrapped,
                     common::marshal::binary::network::create::Output{});
            }

            auto receive_message = TestFixture::create_receive();

            common::communication::ipc::blocking::receive( inbound, receive_message, send_message.correlation);

            TestFixture::compare( send_message, receive_message);
         }














      } // message
   } // gateway
} // casual
