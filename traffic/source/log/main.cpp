/*
 * casual_statisticshandler_main.cpp
 *
 *  Created on: 6 nov 2012
 *      Author: hbergk
 */

#include "common/error.h"
#include "common/environment.h"

#include <iostream>
#include <fstream>

#include "traffic/log/handler.h"
#include "traffic/receiver.h"
#include "traffic/log/file.h"

using namespace casual::traffic::log;

namespace local
{
namespace
{
    std::string getFile()
    {
       return casual::common::environment::directory::domain() + "/txrpt.stat";
    }
}
}

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
		std::ofstream stream( local::getFile(), std::ios_base::app);
		File file( stream);

      casual::traffic::Receiver< File, handle::Notify > receiver( arguments, file);
		receiver.start();

	}
	catch( ...)
	{
		return casual::common::error::handler();

	}
	return 0;
}



