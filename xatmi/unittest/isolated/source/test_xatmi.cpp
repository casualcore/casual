//!
//! test_xatmi.cpp
//!
//! Created on: Jan 7, 2015
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "xatmi.h"



#include "common/mockup/ipc.h"
#include "common/mockup/transform.h"

#include "common/flag.h"

#include "common/message/server.h"
#include "common/message/transaction.h"

#include <map>
#include <vector>

namespace casual
{
   using namespace common;

   namespace xatmi
   {
      namespace local
      {
         namespace
         {
            namespace transform
            {
               struct Lookup
               {
                  Lookup( std::vector< common::message::service::name::lookup::Reply> replies)
                  {
                     for( auto& reply : replies)
                     {
                        m_broker.emplace( reply.service.name, std::move( reply));
                     }
                  }

                  using message_type = common::message::service::name::lookup::Request;
                  using reply_type = common::message::service::name::lookup::Reply;

                  std::vector< ipc::message::Complete> operator () ( message_type message)
                  {
                     common::message::service::name::lookup::Reply reply;

                     auto found = range::find( m_broker, message.requested);

                     if( found)
                     {
                        reply = found->second;
                     }

                     reply.correlation = message.correlation;

                     std::vector< ipc::message::Complete> result;
                     result.emplace_back( marshal::complete( reply));
                     return result;
                  }

                  std::map< std::string, common::message::service::name::lookup::Reply> m_broker;
               };

               namespace transaction
               {

                  struct ClientConnect
                  {
                     using message_type = common::message::transaction::client::connect::Request;
                     using reply_type = common::message::transaction::client::connect::Reply;

                     std::vector< ipc::message::Complete> operator () ( message_type message)
                     {
                        reply_type reply;

                        reply.correlation = message.correlation;
                        reply.domain = "unittest-domain";
                        reply.transactionManagerQueue = common::mockup::ipc::transaction::manager::id();

                        {
                           common::message::transaction::resource::Manager rm;
                           rm.key = "rm-mockup";
                           reply.resourceManagers.push_back( std::move( rm));
                        }

                        std::vector< ipc::message::Complete> result;
                        result.emplace_back( marshal::complete( reply));
                        return result;
                     }
                  };

                  struct Begin
                  {
                     using message_type = common::message::transaction::begin::Request;
                     using reply_type = common::message::transaction::begin::Reply;

                     std::vector< ipc::message::Complete> operator () ( message_type message)
                     {
                        reply_type reply;

                        reply.correlation = message.correlation;
                        reply.process = common::mockup::ipc::transaction::manager::queue().server();
                        reply.state = XA_OK;
                        reply.trid = message.trid;

                        std::vector< ipc::message::Complete> result;
                        result.emplace_back( marshal::complete( reply));
                        return result;
                     }
                  };

                  struct Commit
                  {
                     using message_type = common::message::transaction::commit::Request;
                     using reply_type = common::message::transaction::commit::Reply;

                     std::vector< ipc::message::Complete> operator () ( message_type message)
                     {
                        std::vector< ipc::message::Complete> result;

                        {
                           common::message::transaction::prepare::Reply reply;
                           reply.correlation = message.correlation;
                           reply.resource = 42;
                           reply.state = TX_OK;
                           reply.trid = message.trid;

                           result.emplace_back( marshal::complete( reply));
                        }

                        {
                           common::message::transaction::commit::Reply reply;

                           reply.correlation = message.correlation;
                           reply.process = common::mockup::ipc::transaction::manager::queue().server();
                           reply.state = XA_OK;
                           reply.trid = message.trid;

                           result.emplace_back( marshal::complete( reply));
                        }
                        return result;
                     }
                  };

                  struct Rollback
                  {
                     using message_type = common::message::transaction::rollback::Request;
                     using reply_type = common::message::transaction::rollback::Reply;

                     std::vector< ipc::message::Complete> operator () ( message_type message)
                     {
                        reply_type reply;

                        reply.correlation = message.correlation;
                        reply.process = common::mockup::ipc::transaction::manager::queue().server();
                        reply.state = XA_OK;
                        reply.trid = message.trid;

                        std::vector< ipc::message::Complete> result;
                        result.emplace_back( marshal::complete( reply));
                        return result;
                     }
                  };

               } // transaction


               struct ServiceCall
               {
                  using message_type = common::message::service::callee::Call;
                  using reply_type = common::message::service::Reply;

                  ServiceCall( std::vector< std::pair< std::string, reply_type>> replies)
                  {
                     for( auto& reply : replies)
                     {
                        m_server.emplace( reply.first, std::move( reply.second));
                     }
                  }

                  std::vector< ipc::message::Complete> operator () ( message_type message)
                  {
                     if( common::flag< TPNOREPLY>( message.flags))
                     {
                        //common::log::error << "message.descriptor: " << message.descriptor << " correlation: " << message.correlation << std::endl;
                        return {};
                     }

                     common::message::service::Reply reply;
                     auto found = range::find( m_server, message.service.name);

                     if( found)
                     {
                        reply = found->second;
                     }

                     reply.buffer.memory = message.buffer.memory;
                     reply.buffer.type = message.buffer.type;
                     reply.correlation = message.correlation;
                     reply.transaction.trid = message.trid;
                     reply.descriptor = message.descriptor;


                     std::vector< ipc::message::Complete> result;
                     result.emplace_back( marshal::complete( reply));
                     return result;
                  }

                  std::map< std::string, reply_type> m_server;
               };





               common::mockup::transform::Handler broker( std::vector< common::message::service::name::lookup::Reply> replies)
               {
                  return common::mockup::transform::Handler{ Lookup{ std::move( replies)}, transaction::ClientConnect{}};
               }

               common::mockup::transform::Handler server( std::vector< std::pair< std::string, common::message::service::Reply>> replies)
               {
                  return common::mockup::transform::Handler{ ServiceCall{ std::move( replies)}};
               }

               common::mockup::transform::Handler tm()
               {
                  return common::mockup::transform::Handler{ transaction::Begin{}, transaction::Commit{}, transaction::Rollback{}};
               }

            } // transform


            struct Domain
            {
               //
               // Set up a "broker" that transforms request to replies, set destination to our receive queue
               //
               // Link 'output' from mockup-broker-queue to our "broker"
               //
               Domain()
                  : server{ ipc::receive::id(), transform::server({
                     createService( "service_1"),
                     createService( "timeout_2", TPESVCFAIL)
                  })},
                  broker{ ipc::receive::id(), transform::broker({
                     createName( "service_1", server.id()),
                     createName( "timeout_2", server.id(), std::chrono::milliseconds{ 2})
                  })},
                  link_broker_reply{ mockup::ipc::broker::queue().receive().id(), broker.id()},
                  tm{ ipc::receive::id(), transform::tm()},
                  // link the global mockup-transaction-manager-queue's output to 'our' tm
                  link_tm_reply{ mockup::ipc::transaction::manager::queue().receive().id(), tm.id()}
               {

               }

               mockup::ipc::Router server;
               mockup::ipc::Router broker;
               mockup::ipc::Link link_broker_reply;
               mockup::ipc::Router tm;
               mockup::ipc::Link link_tm_reply;

            private:
               static common::message::service::name::lookup::Reply createName(
                     const std::string& service,
                     common::platform::queue_id_type queue,
                     std::chrono::microseconds timeout = std::chrono::microseconds::zero())
               {
                  common::message::service::name::lookup::Reply reply;
                  reply.process.queue = queue;
                  reply.process.pid = common::process::id();
                  reply.service.name = service;
                  reply.service.timeout = timeout;

                  return reply;
               }

               static std::pair< std::string, common::message::service::Reply> createService(
                     const std::string& service,
                     int value = TPSUCCESS)
               {
                  common::message::service::Reply reply;
                  reply.value = value;

                  return std::make_pair( service, std::move( reply));
               }
            };

         } // <unnamed>
      } // local




      TEST( casual_xatmi, tpalloc_X_OCTET_binary__expect_ok)
      {
         auto buffer = tpalloc( "X_OCTET", "binary", 128);
         EXPECT_TRUE( buffer != nullptr) << "tperrno: " << tperrno;
         tpfree( buffer);
      }

      TEST( casual_xatmi, tpacall_service_null__expect_TPEINVAL)
      {
         char buffer[ 100];
         EXPECT_TRUE( tpacall( nullptr, buffer, 0, 0) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrno;
      }

      TEST( casual_xatmi, tpacall_buffer_null__expect_TPEINVAL)
      {
         EXPECT_TRUE( tpacall( "someServer", nullptr, 0, 0) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrno;
      }

      TEST( casual_xatmi, tpacall_TPNOREPLY_without_TPNOTRAN__expect_TPEINVAL)
      {
         char buffer[ 100];
         EXPECT_TRUE( tpacall( "someServer", buffer, 100, TPNOREPLY) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrno;
      }


      TEST( casual_xatmi, tpcancel_descriptor_42__expect_TPEBADDESC)
      {
         EXPECT_TRUE( tpcancel( 42) == -1);
         EXPECT_TRUE( tperrno == TPEBADDESC) << "tperrno: " << tperrno;
      }


      TEST( casual_xatmi, tx_rollback__no_transaction__expect_TX_PROTOCOL_ERROR)
      {
         EXPECT_TRUE( tx_rollback() == TX_PROTOCOL_ERROR);
      }


      TEST( casual_xatmi, tpgetrply_descriptor_42__expect_TPEBADDESC)
      {
         int descriptor = 42;
         auto buffer = tpalloc( "X_OCTET", "binary", 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpgetrply( &descriptor, &buffer, &len, 0) == -1);
         EXPECT_TRUE( tperrno == TPEBADDESC) << "tperrno: " << tperrno;

         tpfree( buffer);
      }

      TEST( casual_xatmi, tpacall_service_XXX__expect_TPENOENT)
      {
         //
         // Set up a "linked-domain" that transforms request to replies - see above
         //
         local::Domain domain;

         auto buffer = tpalloc( "X_OCTET", "binary", 128);

         EXPECT_TRUE( tpacall( "XXX", buffer, 128, 0) == -1);
         EXPECT_TRUE( tperrno == TPENOENT) << "tperrno: " << common::error::xatmi::error( tperrno);


         tpfree( buffer);
      }


      TEST( casual_xatmi, tpacall_service_service_1_TPNOREPLY_TPNOTRAN__expect_ok)
      {
         //
         // Set up a "linked-domain" that transforms request to replies - see above
         //
         local::Domain domain;

         auto buffer = tpalloc( "X_OCTET", "binary", 128);

         EXPECT_TRUE( tpacall( "service_1", buffer, 128, TPNOREPLY | TPNOTRAN) == 0) << "tperrno: " << common::error::xatmi::error( tperrno);

         tpfree( buffer);
      }


      TEST( casual_xatmi, tpcall_service_service_1__expect_ok)
      {
         //
         // Set up a "linked-domain" that transforms request to replies - see above
         //
         local::Domain domain;

         auto buffer = tpalloc( "X_OCTET", "binary", 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpcall( "service_1", buffer, 128, &buffer, &len, 0) == 0) << "tperrno: " << common::error::xatmi::error( tperrno);

         tpfree( buffer);
      }


      TEST( casual_xatmi, tpacall_service_service_1__no_transaction__tpcancel___expect_ok)
      {
         local::Domain domain;

         auto buffer = tpalloc( "X_OCTET", "binary", 128);

         auto descriptor = tpacall( "service_1", buffer, 128, 0);
         EXPECT_TRUE( descriptor == 1) << "desc: " << descriptor << " tperrno: " << common::error::xatmi::error( tperrno);
         EXPECT_TRUE( tpcancel( descriptor) != -1)  << "tperrno: " << common::error::xatmi::error( tperrno);

         tpfree( buffer);
      }


      TEST( casual_xatmi, tpacall_service_service_1__10_times__no_transaction__tpcancel_all___expect_ok)
      {
         local::Domain domain;

         auto buffer = tpalloc( "X_OCTET", "binary", 128);

         std::vector< int> descriptors( 10);

         for( auto& desc : descriptors)
         {
            desc = tpacall( "service_1", buffer, 128, 0);
            EXPECT_TRUE( desc > 0) << "tperrno: " << common::error::xatmi::error( tperrno);
         }

         std::vector< int> expected_descriptors{ 1, 2, 3, 4, 5, 6, 7 ,8 , 9, 10};
         EXPECT_TRUE( descriptors == expected_descriptors) << "descriptors: " << common::range::make( descriptors);

         for( auto& desc : descriptors)
         {
            EXPECT_TRUE( tpcancel( desc) != -1)  << "tperrno: " << common::error::xatmi::error( tperrno);
         }

         tpfree( buffer);
      }


      TEST( casual_xatmi, tpacall_service_service_1__10_times___tpgetrply_any___expect_ok)
      {
         local::Domain domain;


         auto buffer = tpalloc( "X_OCTET", "binary", 128);

         std::vector< int> descriptors( 10);

         for( auto& desc : descriptors)
         {
            desc = tpacall( "service_1", buffer, 128, 0);
            EXPECT_TRUE( desc > 0) << "tperrno: " << common::error::xatmi::error( tperrno);
         }

         std::vector< int> expected_descriptors{ 1, 2, 3, 4, 5, 6, 7 ,8 , 9, 10};


         std::vector< int> fetched( 10);

         for( auto& fetch : fetched)
         {
            auto len = tptypes( buffer, nullptr, nullptr);
            EXPECT_TRUE( tpgetrply( &fetch, &buffer, &len, TPGETANY) != -1)  << "tperrno: " << common::error::xatmi::error( tperrno);
         }

         EXPECT_TRUE( descriptors == fetched) << "descriptors: " << common::range::make( descriptors) << " fetched: " << common::range::make( fetched);

         tpfree( buffer);
      }


      TEST( casual_xatmi, tx_begin__tpacall_service_service_1__10_times___tpgetrply_all__tx_commit__expect_ok)
      {
         local::Domain domain;

         EXPECT_TRUE( tx_begin() == TX_OK);

         auto buffer = tpalloc( "X_OCTET", "binary", 128);

         std::vector< int> descriptors( 10);

         for( auto& desc : descriptors)
         {
            desc = tpacall( "service_1", buffer, 128, 0);
            EXPECT_TRUE( desc > 0) << "tperrno: " << common::error::xatmi::error( tperrno);
         }

         std::vector< int> expected_descriptors{ 1, 2, 3, 4, 5, 6, 7 ,8 , 9, 10};
         EXPECT_TRUE( descriptors == expected_descriptors) << "descriptors: " << common::range::make( descriptors);

         EXPECT_TRUE( tx_commit() == TX_PROTOCOL_ERROR);

         for( auto& desc : descriptors)
         {
            auto len = tptypes( buffer, nullptr, nullptr);
            EXPECT_TRUE( tpgetrply( &desc, &buffer, &len, 0) != -1)  << "tperrno: " << common::error::xatmi::error( tperrno);
         }

         EXPECT_TRUE( tx_commit() == TX_OK);

         tpfree( buffer);
      }

      TEST( casual_xatmi, tx_begin__tpcall_service_service_1__tx_commit___expect_ok)
      {
         //
         // Set up a "linked-domain" that transforms request to replies - see above
         //
         local::Domain domain;


         EXPECT_TRUE( tx_begin() == TX_OK);

         auto buffer = tpalloc( "X_OCTET", "binary", 128);

         auto len = tptypes( buffer, nullptr, nullptr);
         EXPECT_TRUE( tpcall( "service_1", buffer, 128, &buffer, &len, 0) == 0) << "tperrno: " << common::error::xatmi::error( tperrno);

         EXPECT_TRUE( tx_commit() == TX_OK);

         tpfree( buffer);
      }

      TEST( casual_xatmi, tx_begin__tpacall_service_service_1__tx_commit___expect_TX_PROTOCOL_ERROR)
      {
         //
         // Set up a "linked-domain" that transforms request to replies - see above
         //
         local::Domain domain;


         EXPECT_TRUE( tx_begin() == TX_OK);

         auto buffer = tpalloc( "X_OCTET", "binary", 128);

         EXPECT_TRUE( tpacall( "service_1", buffer, 128, 0) != 0) << "tperrno: " << common::error::xatmi::error( tperrno);

         // can't commit when there are pending replies
         EXPECT_TRUE( tx_commit() == TX_PROTOCOL_ERROR);
         // we can always rollback the transaction
         EXPECT_TRUE( tx_rollback() == TX_OK);

         tpfree( buffer);
      }

      /*
      TEST( casual_xatmi, tpcall_service_timeout_2__expect_TPETIME)
      {
         //
         // Set up a "linked-domain" that transforms request to replies - see above
         //
         local::Domain domain;

         auto buffer = tpalloc( "X_OCTET", "binary", 128);

         long size = 0;

         EXPECT_TRUE( tpcall( "timeout_2", buffer, 128, &buffer, &size, 0) == -1);
         EXPECT_TRUE( tperrno == TPETIME) << "tperrno: " << common::error::xatmi::error( tperrno);

         tpfree( buffer);
      }
      */


   } // xatmi
} // casual
