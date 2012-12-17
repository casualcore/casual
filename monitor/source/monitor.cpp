/*
 * casual_statisticshandler.cpp
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#include "monitor/monitor.h"
#include "common/queue.h"
#include "common/message.h"
#include "common/message_dispatch.h"
#include "utility/logger.h"
#include "utility/trace.h"

#include <vector>
#include <string>

//temp
#include <iostream>

using namespace casual::common;

namespace
{
	//
	// Just temporary
	//
	std::ostream& operator<<( std::ostream& os, const message::monitor::Notify& message)
	{
		os << "parentService: " << message.parentService << std::endl;
		os << "service: " << message.service << std::endl;
		//
		// TODO: etc...
		//
		return os;
	}
}


namespace casual
{
namespace statistics
{
namespace monitor
{
	namespace local
	{
		namespace
		{
			struct Context
			{
                static Context& instance()
                {
                   static Context singleton;
                   return singleton;
                }

    			common::ipc::receive::Queue& receiveQueue()
    			{
    				return m_receiveQueue;
    			}

    			MonitorDB& monitorDB()
    			{
    				return m_monitordb;
    			}

    		private:
    			Context()
    			{
    			}

    			common::ipc::receive::Queue m_receiveQueue;
    			MonitorDB m_monitordb;
    		};
		}
	}

	namespace handle
	{
		void Notify::dispatch( const message_type& message)
		{
			static const std::string cMethodname("Notify::dispatch");
			utility::Trace trace(cMethodname);

			message >> monitorDB;
		}
	}

	Monitor::Monitor(const std::vector<std::string>& arguments) :
			m_receiveQueue( local::Context::instance().receiveQueue()),
			m_monitordb( local::Context::instance().monitorDB())
	{
		static const std::string cMethodname("Monitor::Monitor");
		utility::Trace trace(cMethodname);

		//
		// TODO: Use a correct argumentlist handler
		//
		const std::string name = !arguments.empty() ? arguments.front() : std::string("");
		//
		// Make the key public for others...
		//
		message::monitor::Advertise message;

		message.name = name;
		message.serverId.queue_key = m_receiveQueue.getKey();

		queue::blocking::Writer writer( ipc::getBrokerQueue());
		writer(message);
	}

	Monitor::~Monitor()
	{
		static const std::string cMethodname("Monitor::~Monitor");
		utility::Trace trace(cMethodname);
		//
		// Tell broker that monitor is down...
		//
		message::monitor::Unadvertise message;

		message.serverId.queue_key = m_receiveQueue.getKey();

		queue::blocking::Writer writer( ipc::getBrokerQueue());
		writer(message);

	}

	void Monitor::start()
	{
		static const std::string cMethodname("Monitor::start");
		utility::Trace trace(cMethodname);

		common::dispatch::Handler handler;

		handler.add< handle::Notify>( m_monitordb);

		queue::blocking::Reader queueReader(m_receiveQueue);

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

} // monitor

} // statistics

} // casual

