//!
//! casual_broker.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!

#include "broker/broker.h"
#include "broker/handle.h"
#include "broker/transform.h"

#include "config/domain.h"

#include "common/environment.h"

#include "common/internal/trace.h"
#include "common/internal/log.h"

#include "common/queue.h"
#include "common/message/dispatch.h"
#include "common/process.h"


#include "sf/log.h"


#include <xatmi.h>

#include <fstream>
#include <algorithm>




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
					   //
                  // TODO: ping to see if there are another broker running
                  //
					   common::file::remove( path);
					}

					log::internal::debug << "writing broker queue file: " << path << std::endl;

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
		      common::trace::Outcome trace( "terminate child processes");

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

         auto start = common::platform::clock_type::now();


         broker::QueueBlockingReader blockingReader( m_receiveQueue, m_state);

         //
         // Configure
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
               common::log::information << "failed to open '" << arguments.configurationfile << "' - start anyway..." << std::endl;
            }

            //
            // Set domain name
            //
            environment::domain::name( domain.name);

            common::log::internal::debug << CASUAL_MAKE_NVP( domain);

            m_state = transform::configuration::Domain{}( domain);

         }

         {
            common::trace::internal::Scope trace( "boot domain");

            //
            // boot the domain...
            //
            handle::boot( m_state);
         }





         common::log::internal::debug << "prepare message-pump handlers\n";
         //
         // Prepare message-pump handlers
         //


         message::dispatch::Handler handler{
            handle::Connect{ m_state},
            handle::Disconnect{ m_state},
            handle::Advertise{ m_state},
            handle::Unadvertise{ m_state},
            handle::ServiceLookup{ m_state},
            handle::ACK{ m_state},
            handle::MonitorConnect{ m_state},
            handle::MonitorDisconnect{ m_state},
            handle::transaction::client::Connect{ m_state},
         };



         //
         // Prepare the xatmi-services
         //
         {
            common::server::Arguments arguments{ { common::process::path()}};

            arguments.services.emplace_back( "_broker_listServers", &_broker_listServers, 10, common::server::Service::cNone);
            arguments.services.emplace_back( "_broker_listServices", &_broker_listServices, 10, common::server::Service::cNone);
            arguments.services.emplace_back( "_broker_updateInstances", &_broker_updateInstances, 10, common::server::Service::cNone);

            handler.add( handle::Call{ std::move( arguments), m_state});
         }


         auto end = common::platform::clock_type::now();

         common::log::information << "domain: \'" << common::environment::domain::name() << "\' up and running - " << m_state.instances.size() << " processes - boot time: "
               << std::chrono::duration_cast< std::chrono::milliseconds>( end - start).count() << " ms" << std::endl;



         try
         {
            common::log::internal::debug << "start message pump\n";

            message::dispatch::pump( handler, blockingReader);
         }
         catch( const common::exception::signal::Terminate&)
         {
            auto start = common::platform::clock_type::now();

            common::log::information << "shutting down domain '" << common::environment::domain::name() << "'\n";

            //
            // We will shut down the domain. We start by removing this broker
            //
            m_state.removeInstance( common::process::id());

            try
            {

               common::signal::alarm::Scoped timeout{ 10};

               broker::QueueNonBlockingReader nonblocking( m_receiveQueue, m_state);

               while( m_state.size() )
               {
                  //
                  // conusume until empty
                  //
                  while( handler.dispatch( nonblocking.next()));

                  common::process::sleep( std::chrono::milliseconds( 2));
               }

               auto end = common::platform::clock_type::now();
               common::log::information << "shut down domain '" << common::environment::domain::name() << "' - done - time: "
                     << std::chrono::duration_cast< std::chrono::milliseconds>( end - start).count() << " ms" << std::endl;

            }
            catch( const common::exception::signal::Timeout& exception)
            {
               common::log::error << "failed to get response for terminated servers (# " + std::to_string( m_state.size()) << ") - action: abort" << std::endl;
               throw common::exception::signal::Terminate{};
            }
         }
		}

      void Broker::serverInstances( const std::vector<admin::update::InstancesVO>& instances)
      {
         common::Trace trace( "Broker::serverInstances");

         auto updateInstances = [&]( const admin::update::InstancesVO& value)
               {
                  for( auto&& server : m_state.servers)
                  {
                     if( server.second.alias == value.alias)
                     {
                        m_state.instance( server.second, value.instances);
                     }
                  }
               };
         common::range::for_each( instances, updateInstances);
      }

	} // broker

} // casual





