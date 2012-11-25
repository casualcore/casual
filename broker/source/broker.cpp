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

#include "sf/archive_maker.h"



#include <fstream>
#include <algorithm>

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
         queue::blocking::Reader queueReader( m_receiveQueue);

         bool working = true;

         while( working)
         {

            auto marshal = queueReader.next();

            switch( marshal.type())
            {
               case message::service::Advertise::message_type:
               {
                  message::service::Advertise message;
                  marshal >> message;

                  state::advertiseService( message, m_state);

                  break;
               }
               case message::service::Unadvertise::message_type:
               {
                  message::service::Unadvertise message;
                  marshal >> message;

                  state::unadvertiseService( message, m_state);

                  break;
               }
               case message::server::Disconnect::message_type:
               {
                  message::server::Disconnect message;
                  marshal >> message;

                  state::removeServer( message, m_state);

                  break;
               }
               case message::service::name::lookup::Request::message_type:
               {
                  message::service::name::lookup::Request message;
                  marshal >> message;

                  //
                  // Request service
                  //
                  std::vector< message::service::name::lookup::Reply> response = state::requestService( message, m_state);

                  if( !response.empty())
                  {
                     ipc::send::Queue responseQueue( message.server.queue_key);
                     queue::blocking::Writer writer( responseQueue);

                     writer( response.front());
                  }

                  break;
               }
               case message::service::ACK::message_type:
               {
                  //
                  // Some service is done.
                  // * Release the "lock" on that server
                  // * Check if there are pending service request
                  //
                  message::service::ACK message;
                  marshal >> message;

                  std::vector< state::PendingResponse> pending = state::serviceDone( message, m_state);

                  if( !pending.empty())
                  {
                     ipc::send::Queue responseQueue( pending.front().first);
                     queue::blocking::Writer writer( responseQueue);

                     // TODO: What if we can't write to the queue? The queue is blocking
                     writer( pending.front().second);
                  }

                  break;
               }
               default:
               {
                  utility::logger::error << "message_type: " << " not recognized - action: discard";
                  break;
               }

            }
         }
		}

	} // broker

} // casual





