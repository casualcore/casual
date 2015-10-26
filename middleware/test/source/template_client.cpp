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
#include "buffer/string.h"



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


	auto buffer = tpalloc( CASUAL_STRING, 0, 1024);

	if ( buffer != nullptr)
	{
      const std::string& argument = arguments[ 1];

      std::copy( argument.begin(), argument.end(), buffer);
      buffer[ argument.size()] = '\0';

      long size = 0;
      int cd1 = tpacall( "casual_test1", buffer, 0, 0);
      int cd2 = tpacall( "casual_test2", buffer, 0, 0);

      tpgetrply( &cd1, &buffer, &size, 0);
      std::cout << std::endl << "reply1: " << buffer << std::endl;

      tpgetrply( &cd2, &buffer, &size, 0);
      std::cout << std::endl << "reply2: " << buffer << std::endl;

      tpfree( buffer);
	}
	else
	{
	   std::cout << tperrnostring(tperrno) << std::endl;
	}
}


