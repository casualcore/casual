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
#include <iomanip>
#include <chrono>


#include "xatmi.h"
#include "tx.h"

#include "common/arguments.h"

#include "sf/xatmi_call.h"

void help()
{
   std::cerr << "usage: \n\t" << "test_multicall_client " << " --service --number --argument" << std::endl;
   exit( 1);
}


struct Timeoint
{

   using time_point = decltype( casual::common::platform::clock_type::now());

   Timeoint( time_point time, std::string info) : time( std::move( time)), info( std::move( info)) {}

   time_point time;
   std::string info;
};

int main( int argc, char** argv)
{

   std::string service;
   long calls = 0;
   std::string argument;
   bool transaction = false;

   casual::common::Arguments parser;

   parser.add(
         casual::common::argument::directive( { "-s", "--service"}, "service to call", service),
         casual::common::argument::directive( { "-n", "--number"}, "number of async calls to service", calls),
         casual::common::argument::directive( { "-a", "--argument"}, "argument to the service", argument),
         casual::common::argument::directive( { "-t", "--transaction"}, "call within a transaction", transaction),
         casual::common::argument::directive( { "-h", "--help"}, "shows this help", &help)
   );


   parser.parse( argc, argv);

   if( service.empty())
   {
      std::cerr << "no service provided" << std::endl;
      return 10;
   }

   std::cout << "argument: " << argument << std::endl;

   std::vector< Timeoint> timepoints;

   timepoints.emplace_back( casual::common::platform::clock_type::now(), "start");



   if( transaction)
   {
      tx_begin();
      timepoints.emplace_back( casual::common::platform::clock_type::now(), "tx_begin");
   }

   using Async = casual::sf::xatmi::service::binary::Async;
   Async caller{ service};

   std::vector< Async::receive_type> receivers;

   for( long index = 0; index < calls; ++index )
   {
      caller << CASUAL_MAKE_NVP( argument);
      receivers.push_back( caller());
   }

   timepoints.emplace_back( casual::common::platform::clock_type::now(), "call");


   for( auto&& recive : receivers)
   {
      auto result = recive();
   }

   timepoints.emplace_back( casual::common::platform::clock_type::now(), "receive");

   if( transaction)
   {
      tx_commit();
      timepoints.emplace_back( casual::common::platform::clock_type::now(), "tx_commit");
   }

   //timepoints.emplace_back( casual::common::platform::clock_type::now(), "end");

   typedef std::chrono::microseconds us;


   std::cout << "time spent (us):\n";

   auto current = timepoints.begin();
   auto next = current + 1;

   for( ; next !=  std::end( timepoints); ++current, ++next)
   {
      std::cout << std::left << std::setw(10) << std::setfill( '.') << next->info << ": " <<
            std::right << std::setw( 7) << std::setfill( ' ') << std::chrono::duration_cast< us>( next->time - current->time).count() << "\n";
   }

   std::cout << std::left << std::setw(10) << std::setfill( '.') << "total" << ": " <<
      std::right << std::setw( 7) << std::setfill( ' ') << std::chrono::duration_cast< us>( timepoints.back().time - timepoints.front().time).count() << "\n";



/*
         << "   total......: " << std::setw(7) << std::chrono::duration_cast< us>( timepoints.back().time - timepoints.front().time).count() << "\n"
         << "   tx_begin...: " << std::setw(7) << std::chrono::duration_cast< us>( efter_tx_begin - start).count() << "\n"
         << "   send.......: " << std::setw(7) << std::chrono::duration_cast< us>( efter_call - efter_tx_begin).count() << "\n"
         << "   receive....: " << std::setw(7) << std::chrono::duration_cast< us>( efter_receive - efter_call).count() << "\n"
         << "   tx_commit..: " << std::setw(7) << std::chrono::duration_cast< us>( end - efter_receive).count() << std::endl;
*/


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
