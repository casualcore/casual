//!
//! template_client.cpp
//!
//! Created on: Apr 30, 2012
//!     Author: Lazan
//!


#include <algorithm>
#include <string>
#include <vector>




int main( int argc, char** argv)
{

	std::vector< std::string> arguments;

	std::copy(
		argv,
		argv + argc,
		std::back_inserter( arguments));

	std::vector< std::string>::iterator findIter = std::find(
			arguments.begin(), arguments.end(), "-broker");

	if( findIter != arguments.end() && findIter + 1 != arguments.end())
	{

	}





}


