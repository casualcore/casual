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
	std::ostream& operator<<( std::ostream& os, const message::monitor::NotifyStats& message)
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

    		private:
    			Context()
    			{
    			}

    			common::ipc::receive::Queue m_receiveQueue;
    		};
		}
	}

	namespace handle
	{
		void NotifyStats::dispatch( message_type& message)
		{
			std::cout << message;
		}
	}

Monitor::Monitor(const std::vector<std::string>& arguments) :
		m_receiveQueue( local::Context::instance().receiveQueue())
{

	//
	// Make the key public for others...
	//
	message::monitor::Advertise message;

	message.serverId.queue_key = m_receiveQueue.getKey();

	queue::blocking::Writer writer( ipc::getBrokerQueue());
	writer(message);
}

Monitor::~Monitor()
{
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

    common::dispatch::Handler handler;

    handler.add< handle::NotifyStats>();

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

} // statistics

} // casual

