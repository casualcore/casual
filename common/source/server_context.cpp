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

#include "utility/logger.h"
#include "utility/error.h"



#include "xatmi.h"

#include <algorithm>

extern "C"
{
   int tpsvrinit(int argc, char **argv)
   {
      casual::utility::logger::debug << "internal tpsvrinit called";
      return 0;
   }

   void tpsvrdone()
   {
      casual::utility::logger::debug << "internal tpsvrdone called";
   }
}


namespace casual
{
   namespace common
   {

      namespace server
      {

         namespace local
         {
            namespace transform
            {
               struct Service
               {
                  message::Service operator () ( const Context::service_mapping_type::value_type& value)
                  {
                     message::Service result;

                     result.name = value.first;

                     return result;
                  }
               };


               struct ServiceInformation
               {
                  TPSVCINFO operator () ( message::service::Call& message) const
                  {
                     TPSVCINFO result;

                     strncpy( result.name, message.service.name.data(), sizeof( result.name) );
                     result.data = common::transform::public_buffer( message.buffer.raw());
                     result.len = message.buffer.size();
                     result.cd = message.callDescriptor;
                     result.flags = 0;

                     return result;
                  }

               };
            }
         }


         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }

         void Context::add( service::Context&& context)
         {
            // TODO: add validation
            m_services.emplace( context.m_name, std::move( context));
         }


         int Context::start()
         {
            try
            {
               connect();

               // TODO: Temp
               //int argc = 0;
               //char** argv = 0;

               //tpsvrinit( argc, argv);


               while( true)
               {

                  queue::blocking::Reader queueReader( m_queue);

                  auto marshal = queueReader.next();

                  switch( marshal.type())
                  {
                     case message::service::Call::message_type:
                     {
                        message::service::Call message( buffer::Context::instance().add( buffer::Buffer()));

                        marshal >> message;

                        utility::logger::debug << "service call: " << message.service.name << " cd: " << message.callDescriptor
                              << " caller pid: " << message.reply.pid << " caller queue: " << message.reply.queue_key;


                        handleServiceCall( message);

                        break;
                     }
                     default:
                     {
                        utility::logger::error << "message_type: " << marshal.type() << " not valid";
                        break;
                     }
                  }
               }
            }
            catch( ...)
            {
               //tpsvrdone();

               //
               // Vi need to disconnect from the broker
               //
               disconnect();

               return utility::error::handler();
            }

            return 0;
         }

         Context::Context()
            : m_brokerQueue( calling::Context::instance().brokerQueue()),
              m_queue( calling::Context::instance().receiveQueue())
         {

         }

         void Context::disconnect()
         {
            message::server::Disconnect message;

            //
            // we can't block here...
            //
            queue::non_blocking::Writer writer( m_brokerQueue);
            writer( message);
         }

         void Context::connect()
         {
            //
            // Let the broker know about us, and our services...
            //

            message::service::Advertise message;

            message.serverId.queue_key = m_queue.getKey();

            std::transform(
               m_services.begin(),
               m_services.end(),
               std::back_inserter( message.services),
               local::transform::Service());

            queue::blocking::Writer writer( m_brokerQueue);
            writer( message);

         }


         void Context::handleServiceCall( message::service::Call& message)
         {
            //
            // Prepare for tpreturn.
            //
            int jumpState = setjmp( m_long_jump_buffer);

            if( jumpState == 0)
            {
               //
               // No longjmp has been called, this is the first time in this "service call"
               // Let's call the user service...
               //

               //
               // set the call-correlation
               //
               m_reply.callDescriptor = message.callDescriptor;

               service_mapping_type::iterator findIter = m_services.find( message.service.name);

               if( findIter == m_services.end())
               {
                  throw utility::exception::xatmi::SystemError( "Service [" + message.service.name + "] not present at server - inconsistency between broker and server");
               }

               TPSVCINFO serviceInformation = local::transform::ServiceInformation()( message);

               //
               // Before we call the user function we have to add the buffer to the "buffer-pool"
               //
               buffer::Context::instance().add( std::move( message.buffer));

               findIter->second.call( &serviceInformation);

               //
               // User service returned, not by tpreturn. The standard does not mention this situation, what to do?
               //
               throw utility::exception::xatmi::service::Error( "Service: " + message.service.name + " did not call tpreturn");

            }
            else
            {


               //
               // User has called tpreturn.
               // Send reply to caller
               //
               ipc::send::Queue replyQueue( message.reply.queue_key);
               queue::blocking::Writer replyWriter( replyQueue);
               replyWriter( m_reply);

               //
               // Send ACK to broker
               //
               message::service::ACK ack;
               ack.server = getId();
               ack.service = message.service.name;
               // TODO: ack.time

               // TODO: Switch the above to queue writes, to gain some performance?

               queue::blocking::Writer brokerWriter( m_brokerQueue);
               brokerWriter( ack);

               //
               // Do some cleanup...
               //
               cleanUp();

            }
         }


         void Context::longJumpReturn( int rval, long rcode, char* data, long len, long flags)
         {
            //
            // Prepare buffer.
            // We have to keep state, since there seems not to be any way to send information
            // via longjump...
            //

            m_reply.returnValue = rval;
            m_reply.userReturnCode = rcode;
            m_reply.buffer = buffer::Context::instance().extract( data);

            longjmp( m_long_jump_buffer, 1);
         }

         void Context::advertiseService( const std::string& name, tpservice function)
         {

            //
            // validate
            //

            service_mapping_type::iterator findIter = m_services.find( name);

            if( findIter != m_services.end())
            {
               //
               // service name is already advertised
               // No error if it's the same function
               //
               if( findIter->second.m_function != function)
               {
                  throw utility::exception::xatmi::service::AllreadyAdvertised( "service name: " + name);
               }

            }
            else
            {
               message::service::Advertise message;

               message.serverId = getId();
               // TODO: message.serverPath =
               message.services.push_back( message::Service( name));

               // TODO: make it consistence safe...
               queue::blocking::Writer writer( m_brokerQueue);
               writer( message);

               add( service::Context( name, function));
            }
         }

         void Context::unadvertiseService( const std::string& name)
         {
            if( m_services.erase( name) != 1)
            {
               throw utility::exception::xatmi::service::NoEntry( "service name: " + name);
            }

            message::service::Unadvertise message;
            message.serverId = getId();
            message.services.push_back( message::Service( name));

            queue::blocking::Writer writer( m_brokerQueue);
            writer( message);
         }


         message::server::Id Context::getId()
         {
            message::server::Id result;
            result.queue_key = m_queue.getKey();
            result.pid = utility::platform::getProcessId();

            return result;
         }


         void Context::cleanUp()
         {
            buffer::Context::instance().clear();
         }


      } // server
   } // common
} // casual

