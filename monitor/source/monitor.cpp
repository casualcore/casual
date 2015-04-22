/*
 * casual_statisticshandler.cpp
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#include "monitor/monitor.h"
#include "common/queue.h"
#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/server/handle.h"
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
		   trace::internal::Scope trace( "handle::Notify::dispatch");

			message >> monitorDB;
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

	   trace::internal::Scope trace( "Monitor::Monitor");

		//
		// Connect as a "regular" server
		//
		common::server::connect( {});


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
	   trace::internal::Scope trace( "Monitor::~Monitor");

		try
		{

	      message::dispatch::Handler handler{
	         handle::Notify{ m_monitordb},
	         common::message::handle::Shutdown{},
	      };

         monitor::Transaction transaction( m_monitordb);

         //
         // Consume until the queue is empty or we've got pending replies equal to statistics_batch
         //

         queue::non_blocking::Reader nonBlocking( m_receiveQueue);

         for( auto count = common::platform::statistics_batch;
            handler( nonBlocking.next()) && count > 0; --count)
         {
            ;
         }
		}
		catch( ...)
		{
		   common::error::handler();
		}

		//
		// Test of select
		//
		//common::log::debug << "Statistic logging" << std::endl;
//		auto rowset = m_monitordb.select();
//		for (auto row = rowset.begin(); row != rowset.end(); ++row )
//		{
//			common::log::debug << *row;
//		}
	}

	void Monitor::start()
	{
		trace::internal::Scope trace( "Monitor::start");

		message::dispatch::Handler handler{
		   handle::Notify{ m_monitordb},
		   common::message::handle::Shutdown{},
		};

		queue::blocking::Reader queueReader(m_receiveQueue);

		while( true)
		{

		   monitor::Transaction transaction( m_monitordb);

         //
         // Blocking
         //

         handler( queueReader.next());


         //
         // Consume until the queue is empty or we've got pending replies equal to statistics_batch
         //

         queue::non_blocking::Reader nonBlocking( m_receiveQueue);

         for( auto count = common::platform::statistics_batch;
            handler( nonBlocking.next()) && count > 0; --count)
         {
            ;
         }
      }
	}

} // monitor

} // statistics

} // casual

