//!
//! casual_server_context.cpp
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#include "casual_server_context.h"

#include "casual_message.h"
#include "casual_queue.h"
#include "casual_logger.h"
#include "casual_error.h"

#include "casual_buffer_context.h"
#include "casual_calling_context.h"



#include "xatmi.h"

#include <algorithm>

extern "C"
{
   int tpsvrinit(int argc, char **argv)
   {
      casual::logger::debug << "internal tpsvrinit called";
      return 0;
   }

   void tpsvrdone()
   {
      casual::logger::debug << "internal tpsvrdone called";
   }
}


namespace casual
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
					TPSVCINFO operator () ( message::ServiceCall& message) const
					{
						TPSVCINFO result;

						strncpy( result.name, &message.service.name[ 0], message.service.name.size());
						result.data = message.buffer().raw();
						result.len = message.buffer().size();
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

		void Context::add( const service::Context& context)
		{
			// TODO: add validation
			m_services[ context.m_name] = context;
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
               queue::message_type_type message_type = queueReader.next();

               switch( message_type)
               {
                  case message::ServiceCall::message_type:
                  {
                     message::ServiceCall message( buffer::Context::instance().create());
                     queueReader( message);

                     logger::debug << "service call: " << message.service.name << " cd: " << message.callDescriptor
                           << " caller pid: " << message.reply.pid << " caller queue: " << message.reply.queue_key;


                     handleServiceCall( message);

                     break;
                  }
                  default:
                  {
                     std::cerr << "message_type: " << message_type << " not valid" << std::endl;
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

		      return error::handler();
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
		   message::ServerDisconnect message;

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

			message::ServiceAdvertise message;

			message.serverId.queue_key = m_queue.getKey();

			std::transform(
				m_services.begin(),
				m_services.end(),
				std::back_inserter( message.services),
				local::transform::Service());

			queue::blocking::Writer writer( m_brokerQueue);
			writer( message);

		}


		void Context::handleServiceCall( message::ServiceCall& context)
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
				m_reply.callDescriptor = context.callDescriptor;

				service_mapping_type::iterator findIter = m_services.find( context.service.name);

            if( findIter == m_services.end())
            {
               throw exception::xatmi::SystemError( "Service [" + context.service.name + "] not present at server - inconsistency between broker and server");
            }

				TPSVCINFO serviceInformation = local::transform::ServiceInformation()( context);
				findIter->second.call( &serviceInformation);

				//
				// User service returned, not by tpreturn. The standard does not mention this situation, what to do?
				//
				throw exception::xatmi::service::Error( "Service: " + context.service.name + " did not call tpreturn");

			}
			else
			{


				//
				// User has called tpreturn.
			   // Send reply to caller
				//
				ipc::send::Queue replyQueue( context.reply.queue_key);
				queue::blocking::Writer replyWriter( replyQueue);
				replyWriter( m_reply);

				//
				// Send ACK to broker
				//
				message::ServiceACK ack;
				ack.server = getId();
				ack.service = context.service.name;
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
			m_reply.setBuffer( buffer::Context::instance().getBuffer( data));

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
		         throw exception::xatmi::service::AllreadyAdvertised( "service name: " + name);
		      }

		   }
		   else
		   {
            message::ServiceAdvertise message;

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
		      throw exception::xatmi::service::NoEntry( "service name: " + name);
		   }

		   message::ServiceUnadvertise message;
		   message.serverId = getId();
		   message.services.push_back( message::Service( name));

		   queue::blocking::Writer writer( m_brokerQueue);
		   writer( message);
      }


      message::ServerId Context::getId()
      {
         message::ServerId result;
         result.queue_key = m_queue.getKey();
         result.pid = utility::platform::getProcessId();

         return result;
      }


      void Context::cleanUp()
		{
			buffer::Context::instance().clear();
		}


	}

}


