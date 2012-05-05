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
				void exportBrokerQueueKey( const Q& queue)
				{
					std::ofstream brokerQueueFile( utility::environment::getBrokerQueueFileName().c_str());
					brokerQueueFile << queue.getKey();
				}


			}

		}


		Broker::Broker( const std::vector< std::string>& arguments)
		{


		}

		Broker::~Broker()
		{

		}

		void Broker::start()
		{
			//
			// Make the key public for others...
			//
			local::exportBrokerQueueKey( m_receiveQueue);


		}
	}

}





