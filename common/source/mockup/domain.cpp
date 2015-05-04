//!
//! domain.cpp
//!
//! Created on: Feb 15, 2015
//!     Author: Lazan
//!

#include "common/mockup/domain.h"

#include "common/flag.h"


namespace casual
{
   namespace common
   {
      namespace mockup
      {

         namespace broker
         {

            namespace server
            {
               std::vector< common::ipc::message::Complete> Connect::operator () ( message_type message)
               {
                  reply_type reply;
                  reply.correlation = message.correlation;

                  std::vector< common::ipc::message::Complete> result;
                  result.emplace_back( marshal::complete( reply));
                  return result;
               }

            } // server

            namespace client
            {
               std::vector< common::ipc::message::Complete> Connect::operator () ( message_type message)
               {
                  reply_type reply;

                  reply.correlation = message.correlation;
                  reply.domain = "unittest-domain";
                  reply.transaction_manager = common::mockup::ipc::transaction::manager::id();

                  {
                     common::message::transaction::resource::Manager rm;
                     rm.key = "rm-mockup";
                     reply.resources.push_back( std::move( rm));
                  }

                  std::vector< common::ipc::message::Complete> result;
                  result.emplace_back( marshal::complete( reply));
                  return result;
               }

            } // client


            Lookup::Lookup( std::vector< common::message::service::name::lookup::Reply> replies)
            {
               for( auto& reply : replies)
               {
                  m_broker.emplace( reply.service.name, std::move( reply));
               }
            }

            std::vector< common::ipc::message::Complete> Lookup::operator () ( message_type message)
            {
               common::message::service::name::lookup::Reply reply;

               auto found = range::find( m_broker, message.requested);

               if( found)
               {
                  reply = found->second;
               }

               reply.correlation = message.correlation;

               std::vector< common::ipc::message::Complete> result;
               result.emplace_back( marshal::complete( reply));
               return result;
            }


         } // broker

         namespace service
         {

            Call::Call( std::vector< std::pair< std::string, reply_type>> replies)
            {
               for( auto& reply : replies)
               {
                  m_server.emplace( reply.first, std::move( reply.second));
               }
            }

            std::vector< common::ipc::message::Complete> Call::operator () ( message_type message)
            {
               if( common::flag< TPNOREPLY>( message.flags))
               {
                  //common::log::error << "message.descriptor: " << message.descriptor << " correlation: " << message.correlation << std::endl;
                  return {};
               }

               common::message::service::call::Reply reply;
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


               std::vector< common::ipc::message::Complete> result;
               result.emplace_back( marshal::complete( reply));
               return result;
            }

         } // service

         namespace transaction
         {



            std::vector< common::ipc::message::Complete> Begin::operator () ( message_type message)
            {
               reply_type reply;

               reply.correlation = message.correlation;
               reply.process = common::mockup::ipc::transaction::manager::queue().server();
               reply.state = XA_OK;
               reply.trid = message.trid;

               std::vector< common::ipc::message::Complete> result;
               result.emplace_back( marshal::complete( reply));
               return result;
            }

            std::vector< common::ipc::message::Complete> Commit::operator () ( message_type message)
            {
               std::vector< common::ipc::message::Complete> result;

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

            std::vector< common::ipc::message::Complete> Rollback::operator () ( message_type message)
            {
               reply_type reply;

               reply.correlation = message.correlation;
               reply.process = common::mockup::ipc::transaction::manager::queue().server();
               reply.state = XA_OK;
               reply.trid = message.trid;

               std::vector< common::ipc::message::Complete> result;
               result.emplace_back( marshal::complete( reply));
               return result;
            }

         } // transaction


         namespace create
         {

            namespace lookup
            {
               common::message::service::name::lookup::Reply reply(
                     const std::string& service,
                     platform::queue_id_type queue,
                     std::chrono::microseconds timeout)
               {
                  common::message::service::name::lookup::Reply reply;
                  reply.process.queue = queue;
                  reply.process.pid = common::process::id();
                  reply.service.name = service;
                  reply.service.timeout = timeout;

                  return reply;

               }

            } // lookup

            transform::Handler broker( std::vector< message::service::name::lookup::Reply> replies)
            {
               return transform::Handler{ broker::Lookup{ std::move( replies)}, broker::server::Connect{}, broker::client::Connect{}};
            }

            transform::Handler broker()
            {
               return transform::Handler{ broker::server::Connect{}, broker::client::Connect{}};
            }

            common::mockup::transform::Handler server( std::vector< std::pair< std::string, message::service::call::Reply>> replies)
            {
               return transform::Handler{ service::Call{ std::move( replies)}};
            }

            namespace transaction
            {
               common::mockup::transform::Handler manager()
               {
                  return transform::Handler{ mockup::transaction::Begin{}, mockup::transaction::Commit{}, mockup::transaction::Rollback{}};
               }
            } // transaction


         } // create

      } // mockup
   } // common
} // casual
