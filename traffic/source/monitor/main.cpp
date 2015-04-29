/*
 * casual_statisticshandler_main.cpp
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#include "common/error.h"


#include <iostream>

#include "traffic/monitor/receiver.h"


int main( int argc, char** argv)
{
	try
	{
		std::vector< std::string> arguments;

		std::copy(
			argv,
			argv + argc,
			std::back_inserter( arguments));

		std::cout << "starting" << std::endl;

		casual::traffic::monitor::Receiver receiver( arguments);

		receiver.start();

	}
	catch( ...)
	{
		return casual::common::error::handler();

	}
	return 0;
}



