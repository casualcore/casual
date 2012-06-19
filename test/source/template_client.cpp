//!
//! template_client.cpp
//!
//! Created on: Apr 30, 2012
//!     Author: Lazan
//!


#include <algorithm>
#include <string>
#include <vector>
#include <sstream>

#include "casual_ipc.h"
#include "casual_queue.h"



int main( int argc, char** argv)
{

	std::vector< std::string> arguments;

	std::copy(
		argv,
		argv + argc,
		std::back_inserter( arguments));


	casual::ipc::send::Queue brokerQueue = casual::ipc::getBrokerQueue();
	casual::queue::Writer writer( brokerQueue);


	casual::ipc::receive::Queue myQueue;
	casual::queue::blocking::Reader reader( myQueue);

	casual::message::ServerConnect serverConnect;

	serverConnect.serverId.queue_key = myQueue.getKey();
	serverConnect.serverPath = "/bja/bkalj/bkjls/dkfjslj";

	if( arguments.size() > 1)
	{
		std::istringstream converter( arguments[ 1]);
		std::size_t count;
		converter >> count;

		for( int index = 0; index < count; ++index)
		{
			casual::message::Service service;
			service.name ="sdlkfjslkjdfskldf";
			serverConnect.services.push_back( service);
		}
	}

	writer( serverConnect);

}


