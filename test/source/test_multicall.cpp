//!
//! test_multicall.cpp
//!
//! Created on: Jul 12, 2012
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

   if( arguments.size() < 3)
   {
      std::cerr << "need at least 2 argument" << std::endl;
      return 1;
   }


   std::istringstream converter( arguments.at( 1));
   long calls;
   converter >> calls;



   const std::string& argument = arguments.at(2);
   char* buffer = tpalloc( "STRING", "", argument.size() + 1);

   std::copy( argument.begin(), argument.end(), buffer);
   buffer[ argument.size()] = '\0';

   std::vector< int> callDescriptors;

   for( long index = 0; index < calls; ++index )
   {
      const int cd = tpacall( "casual_test2", buffer, 0, 0);
      if( cd != -1)
      {
         callDescriptors.push_back( cd);
      }
      else
      {
         std::cerr << "tpacall returned -1" << std::endl;
      }
   }



   std::vector< int>::iterator callIter = callDescriptors.begin();

   for( ; callIter != callDescriptors.end(); ++callIter)
   {
      long size = 0;
      if( tpgetrply( &*callIter, &buffer, &size, 0) != -1)
      {
         //std::cout << std::endl << "reply "<< ++reply << ": " << buffer << std::endl;
      }
      else
      {
         std::cerr << "tpgetrply returned -1" << std::endl;
      }

   }

   tpfree( buffer);

}
