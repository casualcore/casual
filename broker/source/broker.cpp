//!
//! casual_broker.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#include "broker/broker.h"
#include "broker/broker_implementation.h"
#include "broker/action.h"

#include "common/environment.h"
#include "common/logger.h"

#include "common/queue.h"
#include "common/message_dispatch.h"
#include "common/process.h"

#include "sf/archive_maker.h"


#include <xatmi.h>

#include <fstream>
#include <algorithm>



//temp
//#include <iostream>



extern "C"
{
   extern void _broker_listServers( TPSVCINFO *serviceInfo);
   extern void _broker_listServices( TPSVCINFO *serviceInfo);
}


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
					if( common::file::exists( path))
					{
					   common::file::remove( path);
					}

					std::ofstream brokerQueueFile( path);
					brokerQueueFile << queue.id();
				}
			}
		}


		Broker::Broker()
			: m_brokerQueueFile( common::environment::getBrokerQueueFileName())
		{

		}

		Broker& Broker::instance()
		{
		   static Broker singleton;
		   return singleton;
		}

		Broker::~Broker()
		{

		   try
		   {
		      common::trace::Exit temp( "terminate child processes");

		      //
		      // We need to terminate all children
		      //
		      for( auto pid : m_state.processes)
		      {
		         process::terminate( pid);
		      }

		      for( auto pid : process::terminated())
		      {
		         logger::information << "shutdown: " << pid;
		      }

		   }
		   catch( ...)
		   {
		      common::error::handler();
		   }
		}

      void Broker::start( const std::vector< std::string>& arguments)
      {
         common::trace::Exit temp( "broker start");

         //
         // Initialize configuration and such
         //
         {
            common::environment::setExecutablePath( arguments.at( 0));

            //
            // Make the key public for others...
            //
            local::exportBrokerQueueKey( m_receiveQueue, m_brokerQueueFile);

            configuration::Settings configuration;

            //
            // Try to find configuration file
            // TODO: you should be able to pass the configurationfile as an argument.
            //
            const std::string configFile = common::environment::getDefaultConfigurationFile();

            if( ! configFile.empty())
            {

               common::logger::information << "broker: using configuration file: " << configFile;

               //
               // Create the reader and deserialize configuration
               //
               auto reader = sf::archive::reader::makeFromFile( configFile);

               reader >> sf::makeNameValuePair( "broker", configuration);

               //
               // Make sure we've got valid configuration
               //
               configuration::validate( configuration);
            }
            else
            {
               common::logger::information << "broker: no configuration file was found - using default";
            }

            common::logger::debug << " m_state.configuration.servers.size(): " << configuration.servers.size();

            {
               common::trace::Exit trace( "start processes");

               //
               // Start the servers...
               // TODO: Need to do more config
               //
               std::for_each(
                     std::begin( configuration.servers),
                     std::end( configuration.servers),
                     action::server::Start( m_state));

               auto terminated = process::terminated();
               common::logger::debug << "#terminated; " << terminated.size();

            }


            {
               common::trace::Exit trace( "transaction monitor connect");

               //
               // We have to wait for TM
               //
               queue::blocking::Reader queueReader( m_receiveQueue);

               handle::TransactionManagerConnect::message_type message;
               queueReader( message);

               handle::TransactionManagerConnect tmConnect( m_state);
               tmConnect.dispatch( message);
            }

         }


         //
         // Prepare message-pump handlers
         //


         message::dispatch::Handler handler;

         handler.add< handle::Connect>( m_state);
         handler.add< handle::Disconnect>( m_state);
         handler.add< handle::Advertise>( m_state);
         handler.add< handle::Unadvertise>( m_state);
         handler.add< handle::ServiceLookup>( m_state);
         handler.add< handle::ACK>( m_state);
         handler.add< handle::MonitorConnect>( m_state);
         handler.add< handle::MonitorDisconnect>( m_state);
         handler.add< handle::TransactionManagerConnect>( m_state);

         //
         // Prepare the xatmi-services
         //
         {
            common::server::Arguments arguments;

            arguments.m_services.emplace_back( "_broker_listServers", &_broker_listServers);
            arguments.m_services.emplace_back( "_broker_listServices", &_broker_listServices);


            arguments.m_argc = 1;
            const char* executable = common::environment::getExecutablePath().c_str();
            arguments.m_argv = &const_cast< char*&>( executable);

            handler.add< handle::Call>( arguments, m_state);
         }

         message::dispatch::pump( handler);

		}

	} // broker

} // casual





