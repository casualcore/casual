//!
//! casual_server_context.cpp
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#include "common/server_context.h"

#include "common/message.h"
#include "common/queue.h"
#include "common/buffer_context.h"
#include "common/calling_context.h"
#include "common/log.h"
#include "common/error.h"



#include "xatmi.h"

#include <algorithm>



namespace casual
{
   namespace common
   {
      namespace server
      {
         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }



         Context::Context()
         {
            trace::internal::Scope log{ "server::Context instansiated"};
         }


         void Context::longJumpReturn( int rval, long rcode, char* data, long len, long flags)
         {
            log::internal::debug << "tpreturn - rval: " << rval << " - rcode: " << rcode << " - data: @" << static_cast< void*>( data) << " - len: " << len << " - flags: " << flags << std::endl;

            //
            // Prepare buffer.
            // We have to keep state, since there seems not to be any way to send information
            // via longjump...
            //

            m_state.reply.returnValue = rval;
            m_state.reply.userReturnCode = rcode;
            m_state.reply.buffer = buffer::Context::instance().extract( data);

            longjmp( m_state.long_jump_buffer, 1);
         }

         void Context::advertiseService( const std::string& name, tpservice function)
         {
            trace::internal::Scope trace{ "server::Context advertise service " + name};

            //
            // validate
            //

            std::string localName = name;

            if( localName.size() >= XATMI_SERVICE_NAME_LENGTH)
            {
               localName.resize( XATMI_SERVICE_NAME_LENGTH - 1);
               log::warning << "service name '" << name << "' truncated to '" << localName << "'";
            }

            auto findIter = m_state.services.find( localName);

            if( findIter != std::end( m_state.services))
            {
               //
               // service name is already advertised
               // No error if it's the same function
               //
               if( findIter->second.function != function)
               {
                  throw common::exception::xatmi::service::AllreadyAdvertised( "service name: " + localName);
               }

            }
            else
            {
               message::service::Advertise message;

               message.server.queue_id = ipc::receive::id();
               message.serverPath = process::path();
               message.services.emplace_back( localName);

               // TODO: make it consistence safe...
               queue::blocking::Writer writer( ipc::broker::id());
               writer( message);

               m_state.services.emplace( localName, Service( localName, function, 0, Service::cJoin));
            }
         }

         void Context::unadvertiseService( const std::string& name)
         {
            trace::internal::Scope log{ "server::Context unadvertise service" + name};

            if( m_state.services.erase( name) != 1)
            {
               throw common::exception::xatmi::service::NoEntry( "service name: " + name);
            }

            message::service::Unadvertise message;
            message.server.queue_id = ipc::receive::id();
            message.services.push_back( message::Service( name));

            queue::blocking::Writer writer( ipc::broker::id());
            writer( message);
         }


         State& Context::state()
         {
            return m_state;
         }


         void Context::finalize()
         {
            buffer::Context::instance().clear();
         }



      } // server

      namespace callee
      {
         namespace handle
         {
            namespace policy
            {
               void Default::connect( message::server::connect::Request& message, const std::vector< transaction::Resource>& resources)
               {

                  //
                  // Let the broker know about us, and our services...
                  //
                  message.server = message::server::Id::current();
                  message.path = common::process::path();
                  blocking_broker_writer brokerWriter;
                  brokerWriter( message);
                  //
                  // Wait for configuration reply
                  //
                  queue::blocking::Reader reader( ipc::receive::queue());
                  message::server::connect::Reply reply;
                  reader( reply);

                  transaction::Context::instance().set( resources);

               }

               void Default::reply( platform::queue_id_type id, message::service::Reply& message)
               {
                  reply_writer writer{ id };
                  writer( message);
               }

               void Default::ack( const message::service::callee::Call& message)
               {
                  message::service::ACK ack;
                  ack.server.queue_id = ipc::receive::id();
                  ack.service = message.service.name;
                  blocking_broker_writer brokerWriter;
                  brokerWriter( ack);
               }


               void Default::disconnect()
               {
                  message::server::Disconnect message;
                  //
                  // we can't block here...
                  //
                  non_blocking_broker_writer brokerWriter;
                  brokerWriter( message);
               }

               void Default::statistics( platform::queue_id_type id, message::monitor::Notify& message)
               {
                  monitor_writer writer{ id};
                  writer( message);
               }

               void Default::transaction( const message::service::callee::Call& message, const server::Service& service)
               {
                  log::internal::debug << "service: " << service << std::endl;

                  switch( service.transaction)
                  {
                     case server::Service::cAuto:
                     {
                        transaction::Context::instance().joinOrStart( message.transaction);

                        break;
                     }
                     case server::Service::cJoin:
                     {
                        if( message.transaction.xid)
                        {
                           transaction::Context::instance().joinOrStart( message.transaction);
                        }

                        break;
                     }
                     case server::Service::cAtomic:
                     {

                        message::Transaction newTransaction;
                        newTransaction.creator = process::id();

                        transaction::Context::instance().joinOrStart( newTransaction);
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

               }

               void Default::transaction( message::service::Reply& message)
               {
                  transaction::Context::instance().finalize( message);
               }

            } // policy
         } // handle
      } // callee
   } // common
} // casual

