/*
 * casual_statisticshandler_main.cpp
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#include "common/error.h"


#include <iostream>
#include <vector>
#include <string>

#include "traffic/monitor/handler.h"
#include "traffic/receiver.h"
#include "traffic/monitor/database.h"

using namespace casual::traffic::monitor;

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

      //
      // Allocate resource
      //
		Database database;
		casual::traffic::Receiver< Database, handle::Notify > receiver( arguments, database);

		receiver.start();

	}
	catch( ...)
	{
		return casual::common::error::handler();

	}
	return 0;
}



