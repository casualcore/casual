/*
 * casual_statisticshandler_main.cpp
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#include "monitor/monitor.h"
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

		std::cout << "starting" << std::endl;

		casual::statistics::monitor::Monitor monitor( arguments);

		monitor.start();

	}
	catch( ...)
	{
		return casual::utility::error::handler();

	}
	return 0;
}



