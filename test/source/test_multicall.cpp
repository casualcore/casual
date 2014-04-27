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
#include "tx.h"

#include "common/arguments.h"

#include "sf/xatmi_call.h"

void help()
{
   std::cerr << "usage: \n\t" << "test_multicall_client " << " --service --number --argument" << std::endl;
   exit( 1);
}


int main( int argc, char** argv)
{

   std::string service;
   long calls = 0;
   std::string argument;

   casual::common::Arguments parser;

   parser.add(
         casual::common::argument::directive( { "-s", "--service"}, "service to call", service),
         casual::common::argument::directive( { "-n", "--number"}, "number of async calls to service", calls),
         casual::common::argument::directive( { "-a", "--argument"}, "argument to the service", argument),
         casual::common::argument::directive( { "-h", "--help"}, "shows this help", &help)
   );


   parser.parse( argc, argv);

   if( service.empty())
   {
      std::cerr << "no service provided" << std::endl;
      return 10;
   }

   std::cout << "argument: " << argument << std::endl;

   tx_begin();


   using Async = casual::sf::xatmi::service::binary::Async;
   Async caller{ service};

   std::vector< Async::receive_type> receivers;

   for( long index = 0; index < calls; ++index )
   {
      caller << CASUAL_MAKE_NVP( argument);
      receivers.push_back( caller());
   }


   for( auto&& recive : receivers)
   {
      auto result = recive();


   }

   tx_commit();

   /*

   for( auto cd : callDescriptors)
   {
      long size = 0;
      if( tpgetrply( &cd, &buffer, &size, 0) != -1)
      {
         //std::cout << std::endl << "cd: "<< cd << ": " << buffer << std::endl;
      }
      else
      {
         std::cerr << "tpgetrply returned -1" << std::endl;
      }

   }


   tpfree( buffer);

   */
}
