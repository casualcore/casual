//!
//! handle.cpp
//!
//! Created on: Dec 13, 2014
//!     Author: Lazan
//!

#include "common/server/handle.h"

#include "common/call/lookup.h"

namespace casual
{
   namespace common
   {
      namespace server
      {
         message::server::connect::Reply connect( const Uuid& identification)
         {
            Connect< queue::policy::NoAction> connect;

            return connect( ipc::receive::queue(), identification, {});
         }

         message::server::connect::Reply connect( ipc::receive::Queue& ipc, std::vector< message::Service> services)
         {
            Connect< queue::policy::NoAction> connect;

            return connect( ipc, std::move( services));
         }

         message::server::connect::Reply connect( ipc::receive::Queue& ipc, std::vector< message::Service> services, const std::vector< transaction::Resource>& resources)
         {
            auto reply = connect( ipc, std::move( services));

            transaction::Context::instance().set( resources);

            return reply;
         }

         namespace handle
         {

            namespace policy
            {
               void Default::connect( ipc::receive::Queue& ipc, std::vector< message::Service> services, const std::vector< transaction::Resource>& resources)
               {

                  //
                  // Let the broker know about us, and our services...
                  //
                  server::connect( ipc, std::move( services), resources);

               }

               void Default::reply( platform::queue_id_type id, message::service::call::Reply& message)
               {
                  reply_writer writer{ id };
                  writer( message);
               }

               void Default::ack( const message::service::call::callee::Request& message)
               {
                  message::service::call::ACK ack;
                  ack.process = process::handle();
                  ack.service = message.service.name;
                  blocking_broker_writer brokerWriter;
                  brokerWriter( ack);
               }


               void Default::statistics( platform::queue_id_type id,  message::traffic::Event& event)
               {
                  log::internal::debug << "policy::Default::statistics - event:" << event << std::endl;

                  try
                  {
                     queue::blocking::Send send;

                     send( id, event);
                  }
                  catch( ...)
                  {
                     error::handler();
                  }
               }

               void Default::transaction( const message::service::call::callee::Request& message, const server::Service& service, const platform::time_point& now)
               {
                  log::internal::debug << "service: " << service << std::endl;

                  //
                  // We keep track of callers transaction (can be null-trid).
                  //
                  transaction::Context::instance().caller = message.trid;

                  switch( service.transaction)
                  {
                     case server::Service::Transaction::automatic:
                     {
                        if( message.trid)
                        {
                           transaction::Context::instance().join( message.trid);
                        }
                        else
                        {
                           transaction::Context::instance().start( now);
                        }
                        break;
                     }
                     case server::Service::Transaction::join:
                     {
                        transaction::Context::instance().join( message.trid);
                        break;
                     }
                     case server::Service::Transaction::atomic:
                     {
                        transaction::Context::instance().start( now);
                        break;
                     }
                     case server::Service::Transaction::none:
                     default:
                     {
                        //
                        // We don't start or join any transactions
                        // (technically we join a null-trid)
                        //
                        transaction::Context::instance().join( transaction::ID{ process::handle()});
                        break;
                     }

                  }

                  //
                  // Set 'global deadline'
                  //
                  transaction::Context::instance().current().timout.set( now, message.service.timeout);

               }

               void Default::transaction( message::service::call::Reply& message, int return_state)
               {
                  transaction::Context::instance().finalize( message, return_state);
               }

               void Default::forward( const message::service::call::callee::Request& message, const State::jump_t& jump)
               {

                  if( transaction::Context::instance().pending())
                  {
                     throw common::exception::xatmi::service::Error( "service: " + message.service.name + " tried to forward with pending transactions");
                  }

                  call::service::Lookup lookup{ jump.forward.service, message::service::lookup::Request::Context::forward};


                  message::service::call::callee::Request request;
                  request.correlation = message.correlation;
                  request.parent = message.service.name;
                  request.descriptor = message.descriptor;
                  request.trid = message.trid;
                  request.process = message.process;
                  request.flags = message.flags;

                  if( jump.buffer.data == nullptr)
                  {
                     request.buffer = buffer::Payload{ nullptr};
                  }
                  else
                  {
                     request.buffer = buffer::pool::Holder::instance().release( jump.buffer.data, jump.buffer.len);
                  }

                  auto target = lookup();

                  if( target.state == message::service::lookup::Reply::State::absent)
                  {
                     throw common::exception::xatmi::service::no::Entry( target.service.name);
                  }
                  else if( target.state ==  message::service::lookup::Reply::State::busy)
                  {
                     //
                     // We wait for service to become idle
                     //
                     target = lookup();
                  }

                  request.service = target.service;


                  log::internal::debug << "policy::Default::forward - request:" << request << std::endl;

                  queue::blocking::Send send;
                  send( target.process.queue, request);
               }

            } // policy

         } // handle
      } // server
   } // common



} // casual
