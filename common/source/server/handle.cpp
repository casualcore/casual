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

               void Default::reply( platform::queue_id_type id, message::service::Reply& message)
               {
                  reply_writer writer{ id };
                  writer( message);
               }

               void Default::ack( const message::service::callee::Call& message)
               {
                  message::service::ACK ack;
                  ack.process = process::handle();
                  ack.service = message.service.name;
                  blocking_broker_writer brokerWriter;
                  brokerWriter( ack);
               }


               void Default::statistics( platform::queue_id_type id, message::monitor::Notify& message)
               {
                  try
                  {
                     monitor_writer writer{ id};
                     writer( message);
                  }
                  catch( ...)
                  {
                     error::handler();
                  }
               }

               void Default::transaction( const message::service::callee::Call& message, const server::Service& service, const platform::time_point& now)
               {
                  log::internal::debug << "service: " << service << std::endl;

                  switch( service.transaction)
                  {
                     case server::Service::cAuto:
                     {
                        transaction::Context::instance().joinOrStart( message.trid);

                        break;
                     }
                     case server::Service::cJoin:
                     {
                        if( message.trid)
                        {
                           transaction::Context::instance().joinOrStart( message.trid);
                        }

                        break;
                     }
                     case server::Service::cAtomic:
                     {
                        transaction::Context::instance().joinOrStart( common::transaction::ID::create());
                        break;
                     }
                     default:
                     {
                        //
                        // We don't start or join any transactions
                        //
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

               void Default::transaction( message::service::Reply& message)
               {
                  transaction::Context::instance().finalize( message);
               }

            } // policy

         } // handle
      } // server
   } // common



} // casual
