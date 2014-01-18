//!
//! casual_broker.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#include "broker/broker.h"
#include "broker/broker_implementation.h"
#include "broker/action.h"

#include "config/domain.h"

#include "common/environment.h"

#include "common/internal/trace.h"
#include "common/internal/log.h"

#include "common/queue.h"
#include "common/message_dispatch.h"
#include "common/process.h"

#include "sf/archive_maker.h"
#include "sf/log.h"


#include <xatmi.h>

#include <fstream>
#include <algorithm>



//temp
//#include <iostream>



extern "C"
{
   extern void _broker_listServers( TPSVCINFO *serviceInfo);
   extern void _broker_listServices( TPSVCINFO *serviceInfo);
   extern void _broker_updateInstances( TPSVCINFO *serviceInfo);
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

					log::debug << "writing broker queue file: " << path << std::endl;

					std::ofstream brokerQueueFile( path);

					if( brokerQueueFile)
					{
                  brokerQueueFile << queue.id() << std::endl;
                  brokerQueueFile.close();
					}
					else
					{
					   throw exception::NotReallySureWhatToNameThisException( "failed to write broker queue file: " + path);
					}
				}
			}
		}



		Broker::Broker()
			: m_brokerQueueFile( common::environment::file::brokerQueue())
		   //: m_brokerQueueFile( "/tmp/crap")
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

		      /*
		      for( auto& instance : m_state.instances)
		      {
		         process::terminate( instance.first);
		      }
		      */


		      for( auto& death : process::lifetime::ended())
		      {
		         sf::log::information << "shutdown: " << death.string() << std::endl;
		      }


		   }
		   catch( ...)
		   {
		      common::error::handler();
		   }
		}

      void Broker::start( const Settings& arguments)
      {
         common::log::information << "broker start";

         broker::QueueBlockingReader blockingReader( m_receiveQueue, m_state);

         //
         // Initialize configuration and such
         //
         {
            common::trace::internal::Scope trace{ "broker configuration"};

            //
            // Make the key public for others...
            //
            local::exportBrokerQueueKey( m_receiveQueue, m_brokerQueueFile);


            config::domain::Domain domain;

            try
            {
               domain = config::domain::get( arguments.configurationfile);
            }
            catch( const exception::FileNotExist& exception)
            {
               common::log::information << "failed to open '" << arguments.configurationfile << "' - starting anyway..." << std::endl;
            }

            //
            // Set domain name
            //
            environment::domain::name( domain.name);

            common::log::internal::debug << CASUAL_MAKE_NVP( domain);

            {
               common::trace::internal::Scope trace( "start processes");



               //
               // Start the servers...
               //
               handle::transaction::ManagerConnect tmConnect( m_state);
               handle::Connect instanceConnect( m_state);
               action::boot::domain( m_state, domain, blockingReader, tmConnect, instanceConnect);

            }

         }


         common::log::internal::debug << "prepare message-pump handlers\n";
         //
         // Prepare message-pump handlers
         //


         message::dispatch::Handler handler;

         handler.add( handle::Connect{ m_state});
         handler.add( handle::Disconnect{ m_state});
         handler.add( handle::Advertise{ m_state});
         handler.add( handle::Unadvertise{ m_state});
         handler.add( handle::ServiceLookup{ m_state});
         handler.add( handle::ACK{ m_state});
         handler.add( handle::MonitorConnect{ m_state});
         handler.add( handle::MonitorDisconnect{ m_state});

         //
         // Prepare the xatmi-services
         //
         {
            common::server::Arguments arguments;

            arguments.m_services.emplace_back( "_broker_listServers", &_broker_listServers);
            arguments.m_services.emplace_back( "_broker_listServices", &_broker_listServices);
            arguments.m_services.emplace_back( "_broker_updateInstances", &_broker_updateInstances);


            arguments.m_argc = 1;
            const char* executable = common::environment::file::executable().c_str();
            arguments.m_argv = &const_cast< char*&>( executable);

            //handler.add( handle::Call{ arguments, m_state});
            handler.add< handle::Call>( arguments, m_state);

         }

         common::log::internal::debug << "start message pump\n";

         message::dispatch::pump( handler, blockingReader);

		}

      void Broker::serverInstances( const std::vector<admin::update::InstancesVO>& instances)
      {
         common::Trace trace( "Broker::serverInstances");

         auto updateInstances = [&]( const admin::update::InstancesVO& value)
               {
                  auto findIter = m_state.servers.find( value.alias);
                  if( findIter != std::end( m_state.servers))
                  {
                     action::update::Instances{ m_state}( findIter->second, value.instances);
                  }
               };

         std::for_each(
            std::begin( instances),
            std::end( instances),
            updateInstances);

      }

	} // broker

} // casual





