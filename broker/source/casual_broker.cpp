//!
//! casual_broker.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#include "casual_broker.h"
#include "casual_broker_implementation.h"

#include "casual_utility_environment.h"
#include "casual_queue.h"

#include "casual_archive_yaml_policy.h"
#include "casual_logger.h"


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
		   const std::string configFile =
		         utility::file::find( utility::environment::getRootPath(), std::regex( "casual_config.yaml" ));

		   if( ! configFile.empty())
		   {

		      logger::information << "using configuration file: " << configFile;

		      // TODO:
		      std::ifstream configStream( configFile);
		      sf::archive::YamlReader reader( configStream);

		      reader >> sf::makeNameValuePair( "broker", m_state.configuration);
		   }
		   else
		   {
		      logger::information << "no configuration file was found - using default";
		   }

		   logger::debug << " m_state.configuration.servers.size(): " << m_state.configuration.servers.size();




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

            queue::message_type_type message_type = queueReader.next();

            switch( message_type)
            {
               case message::ServiceAdvertise::message_type:
               {
                  message::ServiceAdvertise message;
                  queueReader( message);

                  state::advertiseService( message, m_state);

                  break;
               }
               case message::ServiceUnadvertise::message_type:
               {
                  message::ServiceUnadvertise message;
                  queueReader( message);

                  state::unadvertiseService( message, m_state);

                  break;
               }
               case message::ServerDisconnect::message_type:
               {
                  message::ServerDisconnect message;
                  queueReader( message);

                  state::removeServer( message, m_state);

                  break;
               }
               case message::ServiceRequest::message_type:
               {
                  message::ServiceRequest message;
                  queueReader( message);

                  //
                  // Request service
                  //
                  std::vector< message::ServiceResponse> response = state::requestService( message, m_state);

                  if( !response.empty())
                  {
                     ipc::send::Queue responseQueue( message.server.queue_key);
                     queue::blocking::Writer writer( responseQueue);

                     writer( response.front());
                  }

                  break;
               }
               case message::ServiceACK::message_type:
               {
                  //
                  // Some service is done.
                  // * Release the "lock" on that server
                  // * Check if there are pending service request
                  //
                  message::ServiceACK message;
                  queueReader( message);

                  std::vector< state::PendingResponse> pending = state::serviceDone( message, m_state);

                  if( !pending.empty())
                  {
                     ipc::send::Queue responseQueue( pending.front().first);
                     queue::blocking::Writer writer( responseQueue);

                     // TODO: What if we can't write to the queue?
                     writer( pending.front().second);
                  }

                  break;
               }

               case message::MonitorAdvertise::message_type:
               {
            	  //
            	  // TODO: Implement handling
            	  //
                  message::MonitorAdvertise message;
                  queueReader( message);

            	  logger::debug << "Monitor Advertise";
                  break;
               }

               case message::MonitorUnadvertise::message_type:
               {
            	  //
            	  // TODO: Implement handling
            	  //
                  message::MonitorUnadvertise message;
                  queueReader( message);

            	  logger::debug << "Monitor Unadvertise";
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

	} // broker

} // casual





