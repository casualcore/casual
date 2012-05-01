//!
//! casual_broker_main.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "casual_broker.h"
#include "casual_errorhandler.h"




int main( int argc, char** argv)
{
	try
	{
		std::vector< std::string> arguments;

		std::copy(
			argv,
			argv + argc,
			std::back_inserter( arguments));

		casual::Broker broker( arguments);


		broker.start();



	}
	catch( ...)
	{
		return casual::errorHandler();

	}
	return 0;
}
