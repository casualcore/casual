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
#include "common/types.h"
#include "common/logger.h"
#include "common/trace.h"
#include "common/environment.h"


#include <vector>
#include <string>
#include <chrono>

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
		os << "parentService: " << message.parentService << ", ";
		os << "service: " << message.service << ", ";
		os << "callId: " << message.callId.string() << ", ";
		os << "start: " << transform::time( message.start) << ", ";
		// os << "end: " << transform::time( message.end) << ", ";
		os << "difference: " << time_type::duration( message.end - message.start).count() << " usec";
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
			common::Trace trace(cMethodname);

			message >> monitorDB;
		}
	}

	void Monitor::nonBlockingRead( int maxNumberOfMessages) const
	{
		handle::Notify notifier( m_monitordb);

		queue::non_blocking::Reader queueReader( m_receiveQueue);
		for (auto i=0; i < maxNumberOfMessages; i++)
		{
			message::monitor::Notify message;
			if ( queueReader( message))
			{
				notifier.dispatch( message);
			}
			else
			{
				break;
			}
		}

	}

	Monitor::Monitor(const std::vector<std::string>& arguments) :
			m_receiveQueue( local::Context::instance().receiveQueue()),
			m_monitordb( local::Context::instance().monitorDB())
	{
	   //
      // TODO: Use a correct argumentlist handler
      //
      const std::string name = !arguments.empty() ? arguments.front() : std::string("");

	   common::environment::setExecutablePath( name);

		static const std::string cMethodname("Monitor::Monitor");
		common::Trace trace(cMethodname);


		//
		// Make the key public for others...
		//
		message::monitor::Connect message;

		message.path = name;
		message.server.queue_id = m_receiveQueue.id();

		queue::blocking::Writer writer( ipc::getBrokerQueue());
		writer(message);
	}

	Monitor::~Monitor()
	{
		static const std::string cMethodname("Monitor::~Monitor");
		common::Trace trace(cMethodname);

		try
		{
         //
         // Tell broker that monitor is down...
         //
         message::monitor::Disconnect message;

         message.server.queue_id = m_receiveQueue.id();

         queue::blocking::Writer writer( ipc::getBrokerQueue());
         writer(message);
		}
		catch( ...)
		{
		   common::error::handler();
		}

		//
		// Test of select
		//
		common::logger::debug << "Statistic logging";
//		auto rowset = m_monitordb.select();
//		for (auto row = rowset.begin(); row != rowset.end(); ++row )
//		{
//			common::logger::debug << *row;
//		}
	}

	void Monitor::start()
	{
		static const std::string cMethodname("Monitor::start");
		Trace trace(cMethodname);

		message::dispatch::Handler handler;

		handler.add< handle::Notify>( m_monitordb);

		queue::blocking::Reader queueReader(m_receiveQueue);

		bool working = true;

		while( working)
		{
			auto marshal = queueReader.next();

			monitor::Transaction transaction( m_monitordb);

			if( ! handler.dispatch( marshal))
			{
			   common::logger::error << "message_type: " << " not recognized - action: discard";
			}

			nonBlockingRead( common::platform::statistics_batch);
		}
	}

} // monitor

} // statistics

} // casual

