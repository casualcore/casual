//!
//! casual_server_context.cpp
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#include "casual_server_context.h"

#include "casual_message.h"
#include "casual_queue.h"

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
				case message::ServerConnect::message_type:
				{
					message::ServerConnect message;
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


	}

}


