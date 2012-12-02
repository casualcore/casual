//!
//! casual_broker.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#include "broker/broker.h"
#include "broker/broker_implementation.h"

#include "utility/environment.h"
#include "utility/logger.h"

#include "common/queue.h"
#include "common/message_dispatch.h"

#include "sf/archive_maker.h"



#include <fstream>
#include <algorithm>

//temp
#include <iostream>




namespace casual
{
   using namespace common;

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
						std::ifstream exists( path);
						if( exists.good())
						{
							//
							// Remove file
							//
							utility::file::remove( path);
						}
					}
					std::ofstream brokerQueueFile( path);
					brokerQueueFile << queue.getKey();
				}
			}
		}


		Broker::Broker( const std::vector< std::string>& arguments)
			: m_brokerQueueFile( utility::environment::getBrokerQueueFileName())
		{

		   //
		   // Try to find configuration file
		   // TODO: you should be able to pass the configurationfile as an argument.
		   //
		   const std::string configFile = utility::environment::getDefaultConfigurationFile();

		   if( ! configFile.empty())
		   {

		      utility::logger::information << "broker: using configuration file: " << configFile;

		      //
		      // Create the reader and deserialize configuration
		      //
		      auto reader = sf::archive::reader::makeFromFile( configFile);
		      reader >> sf::makeNameValuePair( "broker", m_state.configuration);

		      //
		      // Make sure we've got valid configuration
		      //
		      configuration::validate( m_state.configuration);
		   }
		   else
		   {
		      utility::logger::information << "broker: no configuration file was found - using default";
		   }

		   utility::logger::debug << " m_state.configuration.servers.size(): " << m_state.configuration.servers.size();




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
         common::dispatch::Handler handler;

         handler.add< handle::Advertise>( m_state);
         handler.add< handle::Disconnect>( m_state);
         handler.add< handle::Unadvertise>( m_state);
         handler.add< handle::ServiceLookup>( m_state);
         handler.add< handle::ACK>( m_state);


         queue::blocking::Reader queueReader( m_receiveQueue);

         bool working = true;

         while( working)
         {

            auto marshal = queueReader.next();

            if( ! handler.dispatch( marshal))
            {
               utility::logger::error << "message_type: " << " not recognized - action: discard";
            }

         }
		}

	} // broker

} // casual





