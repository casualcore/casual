//!
//! casual_broker_main.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "broker/broker.h"


#include "utility/error.h"


#include <iostream>


int main( int argc, char** argv)
{
	try
	{
		std::vector< std::string> arguments;

		std::copy(
			argv,
			argv + argc,
			std::back_inserter( arguments));

		std::cout << "Instantiate" << std::endl;

		casual::broker::Broker broker( arguments);

		std::cout << "starting" << std::endl;

		broker.start();



	}
	catch( ...)
	{
		return casual::error::handler();

	}
	return 0;
}
