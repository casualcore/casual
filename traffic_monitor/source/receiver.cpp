/*
 * casual_statisticshandler.cpp
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

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

#include "traffic_monitor/receiver.h"

#include "traffic_monitor/serviceentryvo.h"

using namespace casual::common;

namespace casual
{
namespace traffic_monitor
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


    			Database& monitorDB()
    			{
    				return m_database;
    			}

    		private:
    			Context()
    			{
    			}

    			Database m_database;
    		};
		}
	}

	namespace handle
	{
		void Notify::operator () ( const message_type& message)
		{
		   trace::internal::Scope trace( "handle::Notify::dispatch");

			message >> database;
		}
	}


	Receiver::Receiver(const std::vector<std::string>& arguments) :
			m_receiveQueue( common::ipc::receive::queue()),
			m_database( local::Context::instance().monitorDB())
	{
	   //
      // TODO: Use a correct argumentlist handler
      //
      const std::string name = ! arguments.empty() ? arguments.front() : std::string("");

	   common::process::path( name);

	   trace::internal::Scope trace( "Receiver::Receiver");

		//
		// Connect as a "regular" server
		//
		common::server::connect( {});


		//
		// Make the key public for others...
		//
		message::traffic_monitor::Connect message;

		message.path = name;
		message.process = common::process::handle();

		queue::blocking::Writer writer( ipc::broker::id());
		writer(message);
	}

	Receiver::~Receiver()
	{
	   trace::internal::Scope trace( "Receiver::~Receiver");

		try
		{

	      message::dispatch::Handler handler{
	         handle::Notify{ m_database},
	         common::message::handle::Shutdown{},
	      };

         traffic_monitor::Transaction transaction( m_database);

         //
         // Consume until the queue is empty or we've got pending replies equal to statistics_batch
         //

         queue::non_blocking::Reader nonBlocking( m_receiveQueue);

         for( auto count = common::platform::batch::statistics;
            handler( nonBlocking.next()) && count > 0; --count)
         {
            ;
         }
		}
		catch( ...)
		{
		   common::error::handler();
		   return;
		}

		//
		// Test of select
		//
//		common::log::debug << "Statistic logging" << std::endl;
//		auto rowset = m_monitordb.select();
//		for (auto row : rowset)
//		{
//			common::log::debug << row;
//		}
	}

	void Receiver::start()
	{
		trace::internal::Scope trace( "Receiver::start");

		message::dispatch::Handler handler{
		   handle::Notify{ m_database},
		   common::message::handle::Shutdown{},
		};

		queue::blocking::Reader queueReader(m_receiveQueue);

		while( true)
		{

		   traffic_monitor::Transaction transaction( m_database);

         //
         // Blocking
         //

         handler( queueReader.next());


         //
         // Consume until the queue is empty or we've got pending replies equal to statistics_batch
         //

         queue::non_blocking::Reader nonBlocking( m_receiveQueue);

         for( auto count = common::platform::batch::statistics;
            handler( nonBlocking.next()) && count > 0; --count)
         {
            ;
         }
      }
	}

} // monitor

} // casual

