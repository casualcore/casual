//!
//! casual_broker.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#include "casual_broker.h"
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

					std::cout << "---- Message Recived ----\n";
					std::cout << "message_type: " << message_type << std::endl;
					std::cout << "queue_key: " << message.queue_key << std::endl;
					std::cout << "serverPath: " << message.serverPath << std::endl;
					std::cout << "services.size(): " << message.services.size() << std::endl;


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





