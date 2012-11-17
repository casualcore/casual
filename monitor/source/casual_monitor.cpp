/*
 * casual_statisticshandler.cpp
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#include "casual_monitor.h"
#include "casual_queue.h"
#include "casual_logger.h"

#include <vector>
#include <string>

//temp
#include <iostream>

namespace
{
	//
	// Just temporary
	//
	std::ostream& operator<<( std::ostream& os, const casual::message::MonitorCall& message)
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


Context::Context() : m_brokerQueue( ipc::getBrokerQueue())
{

}

ipc::send::Queue& Context::brokerQueue()
{
	return m_brokerQueue;
}
ipc::receive::Queue& Context::receiveQueue()
{
	return m_receiveQueue;
}

Context& Context::instance()
{
	static Context singelton;
	return singelton;
}

Monitor::Monitor(const std::vector<std::string>& arguments) :
		m_brokerQueue( Context::instance().brokerQueue()),
		m_receiveQueue( Context::instance().receiveQueue())
{

	//
	// Make the key public for others...
	//
	message::MonitorAdvertise message;

	message.serverId.queue_key = m_receiveQueue.getKey();

	queue::blocking::Writer writer( m_brokerQueue);
	writer(message);
}

Monitor::~Monitor()
{
	//
	// Tell broker that monitor is down...
	//
	message::MonitorUnadvertise message;

	message.serverId.queue_key = m_receiveQueue.getKey();

	queue::blocking::Writer writer( m_brokerQueue);
	writer(message);

}

void Monitor::start()
{
	queue::blocking::Reader queueReader(m_receiveQueue);

	bool working = true;

	while (working) {

		queue::message_type_type message_type = queueReader.next();

		switch (message_type)
		{
		case message::MonitorCall::message_type:
		{
			message::MonitorCall message;
			queueReader(message);

			std::cout << message;

			break;
		}

		default:
		{
			std::cerr << "message_type: " << message_type << " not valid"
					<< std::endl;
			break;
		}

		}
	}
}

} // broker

} // casual

