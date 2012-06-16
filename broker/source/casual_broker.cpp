//!
//! casual_broker.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#include "casual_broker.h"
#include "casual_broker_transform.h"

#include "casual_utility_environment.h"
#include "casual_queue.h"




#include <fstream>


//temp
#include <iostream>



namespace casual
{
	namespace broker
	{
		namespace local
		{
			namespace
			{
				template< typename Q>
				void exportBrokerQueueKey( const Q& queue, const std::string& path)
				{
					{
						//
						// Check if file already exists
						//
						std::ifstream exists( path.c_str());
						if( exists.good())
						{
							//
							// Remove file
							//
							utility::file::remove( path);
						}
					}
					std::ofstream brokerQueueFile( path.c_str());
					brokerQueueFile << queue.getKey();
				}





			}
		}


		Broker::Broker( const std::vector< std::string>& arguments)
			: m_brokerQueueFile( utility::environment::getBrokerQueueFileName())
		{
			//
			// Make the key public for others...
			//
			local::exportBrokerQueueKey( m_receiveQueue, m_brokerQueueFile);

		}

		Broker::~Broker()
		{

		}

		void Broker::start()
		{
			queue::Reader queueReader( m_receiveQueue);

			while( true)
			{
				std::cout << "---- Reading queue  ----" << std::endl;

				queue::Reader::message_type_type message_type = queueReader.next();

				switch( message_type)
				{
				case message::ServerConnect::message_type:
				{
					message::ServerConnect message;
					queueReader( message);

					Servers::iterator serverIterator = m_servers.insert( m_servers.begin(), transform::Server()( message));

					std::for_each(
						message.services.begin(),
						message.services.end(),
						transform::Service( serverIterator, m_services));

					break;
				}
				case message::ServiceRequest::message_type:
				{
					message::ServiceRequest message;
					queueReader( message);

					message::ServiceResponse responseMessage;
					responseMessage.requested = message.requested;

					service_mapping_type::iterator findIter = m_services.find( message.requested);
					if( findIter!= m_services.end())
					{
						transform::Server transform;
						Server& server = findIter->second.nextServer();

						responseMessage.server.push_back( transform( findIter->second.nextServer()));
					}

					ipc::send::Queue responseQueue( message.server.queue_key);
					queue::Writer writer( responseQueue);

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
	}

}





