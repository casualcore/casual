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
#include "common/logger.h"
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
            trace::Exit log{ "server::Context instansiated"};
         }


         void Context::longJumpReturn( int rval, long rcode, char* data, long len, long flags)
         {
            logger::debug << "tpreturn - rval: " << rval << " - rcode: " << rcode << " - data: @" << static_cast< void*>( data) << " - len: " << len << " - flags: " << flags;

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
            trace::Exit log{ "server::Context advertise service " + name};

            //
            // validate
            //

            std::string localName = name;

            if( localName.size() >= XATMI_SERVICE_NAME_LENGTH)
            {
               localName.resize( XATMI_SERVICE_NAME_LENGTH - 1);
               logger::warning << "service name '" << name << "' truncated to '" << localName << "'";
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

               message.server.queue_id = ipc::getReceiveQueue().id();
               // TODO: message.serverPath =
               message.services.emplace_back( localName);

               // TODO: make it consistence safe...
               queue::blocking::Writer writer( ipc::getBrokerQueue());
               writer( message);

               m_state.services.emplace( localName, Service( localName, function));
            }
         }

         void Context::unadvertiseService( const std::string& name)
         {
            trace::Exit log{ "server::Context unadvertise service" + name};

            if( m_state.services.erase( name) != 1)
            {
               throw common::exception::xatmi::service::NoEntry( "service name: " + name);
            }

            message::service::Unadvertise message;
            message.server.queue_id = ipc::getReceiveQueue().id();
            message.services.push_back( message::Service( name));

            queue::blocking::Writer writer( ipc::getBrokerQueue());
            writer( message);
         }


         State& Context::getState()
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
               message::server::Configuration Default::connect( message::server::Connect& message)
               {
                  //
                  // Let the broker know about us, and our services...
                  //
                  message.server.queue_id = ipc::getReceiveQueue().id();
                  message.path = common::environment::file::executable();
                  blocking_broker_writer brokerWriter;
                  brokerWriter( message);
                  //
                  // Wait for configuration reply
                  //
                  queue::blocking::Reader reader( ipc::getReceiveQueue());
                  message::server::Configuration configuration;
                  reader( configuration);
                  return configuration;
               }

               void Default::reply( platform::queue_id_type id, message::service::Reply& message)
               {
                  reply_writer writer{ id };
                  writer( message);
               }

               void Default::ack( const message::service::callee::Call& message)
               {
                  message::service::ACK ack;
                  ack.server.queue_id = ipc::getReceiveQueue().id();
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

            } // policy
         } // handle
      } // callee
   } // common
} // casual

