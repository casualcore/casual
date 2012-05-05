//!
//! casual_broker.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#include "casual_broker.h"
#include "casual_utility_environment.h"

#include <fstream>



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



		}
	}

}





