//!
//! casual_server_context.cpp
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#include "casual_server_context.h"

#include "casual_message.h"
#include "casual_queue.h"
#include "casual_buffer.h"


#include "xatmi.h"


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
					TPSVCINFO operator () ( message::ServiceCall& message)
					{
						TPSVCINFO result;

						strncpy( result.name, &message.service[ 0], message.service.size());
						result.data = message.buffer().raw();
						result.len = message.buffer().size();
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
			connect();

			while( true)
			{
				//std::cout << "---- Reading queue  ----" << std::endl;

				queue::Reader queueReader( m_queue);
				queue::Reader::message_type_type message_type = queueReader.next();

				switch( message_type)
				{
				case message::ServiceCall::message_type:
				{
					message::ServiceCall message( buffer::Holder::instance().create());
					queueReader( message);

					break;
				}
				default:
				{
					std::cerr << "message_type: " << message_type << " not valid" << std::endl;
					break;
				}


				}
			}

			return 0;
		}

		Context::Context()
			: m_brokerQueue( ipc::getBrokerQueue())
		{

		}

		void Context::connect()
		{
			//
			// Let the broker know about us, and our services...
			//

			message::ServerConnect message;

			message.serverId.queue_key = m_queue.getKey();

			std::transform(
				m_services.begin(),
				m_services.end(),
				std::back_inserter( message.services),
				local::transform::Service());

			queue::Writer writer( m_brokerQueue);
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
				// No longjmp has been called, this is the first time in this context
				// Let's call the user service...
				//
				service::Context& service = getService( context.service);

				TPSVCINFO serviceInformation = local::transform::ServiceInformation()( context);
				service.call( &serviceInformation);

			}
			else
			{
				//
				// User has called tpreturn.
				//
				ipc::send::Queue replyQueue( context.reply.queue_key);
				queue::Writer writer( replyQueue);

				writer( m_reply);

				//
				// TODO: Do some cleanup...
				//

			}
		}

		service::Context& Context::getService( const std::string& name)
		{
			service_mapping_type::iterator findIter = m_services.find( name);

			if( findIter == m_services.end())
			{
				throw exception::NotReallySureWhatToNameThisException();
			}

			return findIter->second;
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
			m_reply.setBuffer( buffer::Holder::instance().getBuffer( data));

			longjmp( m_long_jump_buffer, 1);
		}

		void Context::cleanUp()
		{
			m_reply.clearBuffer();
			buffer::Holder::instance().clear();
		}


	}

}


