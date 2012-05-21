//!
//! template_client.cpp
//!
//! Created on: Apr 30, 2012
//!     Author: Lazan
//!


#include <algorithm>
#include <string>
#include <vector>

#include "casual_ipc.h"



int main( int argc, char** argv)
{

	std::vector< std::string> arguments;

	std::copy(
		argv,
		argv + argc,
		std::back_inserter( arguments));


	casual::ipc::send::Queue brokerQueue = casual::ipc::getBrokerQueue();


	casual::ipc::message::Transport message;

	message.m_payload.m_type = 10;

	std::copy(
			arguments.at( 1).begin(),
			arguments.at( 1).end(),
			message.m_payload.m_payload);

	brokerQueue( message);


}


