//!
//! casual_broker_main.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "casual_broker.h"
#include "casual_error.h"




int main( int argc, char** argv)
{
	try
	{
		std::vector< std::string> arguments;

		std::copy(
			argv,
			argv + argc,
			std::back_inserter( arguments));

		casual::broker::Broker broker( arguments);


		broker.start();



	}
	catch( ...)
	{
		return casual::error::handler();

	}
	return 0;
}
