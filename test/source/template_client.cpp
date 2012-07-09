//!
//! template_client.cpp
//!
//! Created on: Apr 30, 2012
//!     Author: Lazan
//!


#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "xatmi.h"



int main( int argc, char** argv)
{

	std::vector< std::string> arguments;

	std::copy(
		argv,
		argv + argc,
		std::back_inserter( arguments));

	if( arguments.size() < 2)
	{
	   std::cerr << "need at least 1 argument" << std::endl;
	   return 1;
	}


	char* buffer = tpalloc( "STRING", "", 1024);

	const std::string& argument = arguments[ 1];

	std::copy( argument.begin(), argument.end(), buffer);
	buffer[ argument.size()] = '\0';


	long size = 0;
	tpcall( "casual_test1", buffer, 0, &buffer, &size, 0);

	std::cout << std::endl << "reply: " << buffer << std::endl;

	tpfree( buffer);


}


