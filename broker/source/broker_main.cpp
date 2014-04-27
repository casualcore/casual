//!
//! casual_broker_main.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include "broker/broker.h"


#include "common/error.h"
#include "common/arguments.h"


#include <iostream>

using namespace casual;

int main( int argc, char** argv)
{
	try
	{

	   broker::Settings settings;

	   {
	      common::Arguments parser;

	      parser.add(
	            common::argument::directive( {"-c", "--configuration-file"}, "domain configuration file", settings.configurationfile)
	      );

	      parser.parse( argc, argv);

	      common::process::path( parser.processName());
	   }

		casual::broker::Broker::instance().start( settings);

	}
	catch( ...)
	{
		return casual::common::error::handler();

	}
	return 0;
}
