/*
 * casual_statisticshandler_main.cpp
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#include "casual_monitor.h"
#include "casual_error.h"


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

		casual::statistics::Monitor monitor( arguments);

		std::cout << "starting" << std::endl;

		monitor.start();

	}
	catch( ...)
	{
		return casual::error::handler();

	}
	return 0;
}



