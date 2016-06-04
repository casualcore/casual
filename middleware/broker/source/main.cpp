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
	      common::Arguments parser{ "casual broker", {}

	      };

	      parser.parse( argc, argv);

	   }

		casual::broker::Broker broker( std::move( settings));
		broker.start();

	}
	catch( ...)
	{
		return casual::common::error::handler();

	}
	return 0;
}
