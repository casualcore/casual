/*
 * casual_statisticshandler.cpp
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#include "monitor/monitor.h"
#include "common/queue.h"
#include "common/message/dispatch.h"
#include "common/platform.h"
#include "common/process.h"
#include "common/error.h"
#include "common/log.h"
#include "common/trace.h"
#include "common/environment.h"
#include "common/chronology.h"


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
   /*
	std::ostream& operator<<( std::ostream& os, const message::monitor::Notify& message)
	{
		os << "parentService: " << message.parentService << ", ";
		os << "service: " << message.service << ", ";
		os << "callId: " << message.callId.string() << ", ";
		os << "start: " << chronology::local( message.start) << ", ";
		// os << "end: " << transform::time( message.end) << ", ";
		os << "difference: " << platform::time_type::duration( message.end - message.start).count() << " usec";
		//
		// TODO: etc...
		//
		return os;
	}
	*/
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


    			MonitorDB& monitorDB()
    			{
    				return m_monitordb;
    			}

    		private:
    			Context()
    			{
    			}

    			MonitorDB m_monitordb;
    		};
		}
	}

	namespace handle
	{
		void Notify::operator () ( const message_type& message)
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
				notifier( message);
			}
			else
			{
				break;
			}
		}

	}

	Monitor::Monitor(const std::vector<std::string>& arguments) :
			m_receiveQueue( common::ipc::receive::queue()),
			m_monitordb( local::Context::instance().monitorDB())
	{
	   //
      // TODO: Use a correct argumentlist handler
      //
      const std::string name = ! arguments.empty() ? arguments.front() : std::string("");

	   common::process::path( name);

		static const std::string cMethodname("Monitor::Monitor");
		common::Trace trace(cMethodname);


		//
		// Make the key public for others...
		//
		message::monitor::Connect message;

		message.path = name;
		message.process = common::process::handle();

		queue::blocking::Writer writer( ipc::broker::id());
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
         message.process = common::process::handle();

         queue::blocking::Writer writer( ipc::broker::id());
         writer(message);
		}
		catch( ...)
		{
		   common::error::handler();
		}

		//
		// Test of select
		//
		common::log::debug << "Statistic logging" << std::endl;
//		auto rowset = m_monitordb.select();
//		for (auto row = rowset.begin(); row != rowset.end(); ++row )
//		{
//			common::log::debug << *row;
//		}
	}

	void Monitor::start()
	{
		static const std::string cMethodname("Monitor::start");
		Trace trace(cMethodname);

		message::dispatch::Handler handler{
		   handle::Notify{ m_monitordb}
		};

		queue::blocking::Reader queueReader(m_receiveQueue);

		bool working = true;

		while( working)
		{
			auto marshal = queueReader.next();

			monitor::Transaction transaction( m_monitordb);

			if( ! handler( marshal))
			{
			   common::log::error << "message_type: " << " not recognized - action: discard" << std::endl;
			}

			nonBlockingRead( common::platform::statistics_batch);
		}
	}

} // monitor

} // statistics

} // casual

