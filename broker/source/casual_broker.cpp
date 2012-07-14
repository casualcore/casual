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

                  std::vector< state::PendingResponse> response = state::serviceDone( message, m_state);

                  if( !response.empty())
                  {
                     ipc::send::Queue responseQueue( response.front().first);
                     queue::blocking::Writer writer( responseQueue);

                     writer( response.front().second);
                  }

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





