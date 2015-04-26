//!
//! handle.cpp
//!
//! Created on: Dec 13, 2014
//!     Author: Lazan
//!

#include "common/server/handle.h"

#include "common/call/timeout.h"

namespace casual
{
   namespace common
   {
      namespace server
      {

         message::server::connect::Reply connect( std::vector< message::Service> services)
         {
            Connect< queue::policy::NoAction> connect;

            return connect( std::move( services));
         }

         message::server::connect::Reply connect( std::vector< message::Service> services, const std::vector< transaction::Resource>& resources)
         {
            auto reply = connect( std::move( services));

            transaction::Context::instance().set( resources);

            return reply;
         }

         namespace handle
         {

            namespace policy
            {
               void Default::connect( std::vector< message::Service> services, const std::vector< transaction::Resource>& resources)
               {

                  //
                  // Let the broker know about us, and our services...
                  //
                  server::connect( std::move( services), resources);

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


               void Default::statistics( platform::queue_id_type id, message::monitor::Notify& message)
               {
                  log::internal::debug << "policy::Default::statistics - message:" << message << std::endl;

                  try
                  {
                     queue::blocking::Send send;

                     send( id, message);
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
                  // We add callers transaction (can be null-trid).
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
                           transaction::Context::instance().start();
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
                        transaction::Context::instance().start();
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
                  // Add 'global' deadline
                  //
                  call::Timeout::instance().add(
                        transaction::Context::instance().current().trid,
                        message.service.timeout,
                        now);

               }

               void Default::transaction( message::service::call::Reply& message, int return_state)
               {
                  transaction::Context::instance().finalize( message, return_state);
               }

            } // policy

         } // handle
      } // server
   } // common



} // casual
