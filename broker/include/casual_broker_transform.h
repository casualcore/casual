//!
//! casual_broker_transform.h
//!
//! Created on: Jun 15, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_BROKER_TRANSFORM_H_
#define CASUAL_BROKER_TRANSFORM_H_

#include "casual_broker.h"


namespace casual
{

	namespace broker
	{
		namespace transform
		{
			struct Server
			{
				casual::broker::Server operator () ( const message::ServerConnect& message) const
				{
					casual::broker::Server result;

					result.m_path = message.serverPath;
					result.m_pid = message.serverId.pid;
					result.m_queue_key = message.serverId.pid;

					return result;
				}

				casual::message::ServerId operator () ( casual::broker::Server& value) const
				{
					casual::message::ServerId result;

					result.pid = value.m_pid;
					result.queue_key = value.m_queue_key;

					++value.m_requested;

					return result;
				}
			};

			struct Service
			{
				Service( Servers::iterator serverIterator, Broker::service_mapping_type& serviceMapping)
					: m_serverIterator( serverIterator), m_serviceMapping( serviceMapping) {}

				void operator() ( const message::Service& service)
				{
					Broker::service_mapping_type::iterator findIter =  m_serviceMapping.find( service.name);

					if( findIter == m_serviceMapping.end())
					{
						findIter = m_serviceMapping.insert( std::make_pair( service.name, broker::Service( service.name))).first;
					}

					findIter->second.add( m_serverIterator);
				}

			private:
				Servers::iterator m_serverIterator;
				Broker::service_mapping_type& m_serviceMapping;
			};


		}



	}


}


#endif /* CASUAL_BROKER_TRANSFORM_H_ */
