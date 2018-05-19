//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "xatmi.h"
#include "tx.h"

#include "common/argument.h"
#include "common/chronology.h"
#include "common/exception/handle.h"

#include "serviceframework/service/protocol/call.h"




#include <algorithm>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <chrono>


struct Timeoint
{

   using time_point = decltype( casual::common::platform::time::clock::type::now());

   Timeoint( time_point time, std::string info) : time( std::move( time)), info( std::move( info)) {}

   time_point time;
   std::string info;
};



struct Settings
{
   std::string service;
   long calls = 1;
   std::string argument;
   bool transaction = false;
   long iterations = 1;
   bool rollback = false;
};

struct Transaction
{
   Transaction( Settings& settings, std::vector< Timeoint>& timepoint) : m_settings( settings), m_timepoint( timepoint)
   {
      if( settings.transaction)
      {
         tx_begin();
         m_timepoint.emplace_back( casual::common::platform::time::clock::type::now(), "tx_begin");
      }
   }

   ~Transaction()
   {
      if( m_settings.transaction)
      {
         if( m_settings.rollback || std::uncaught_exception())
         {
            tx_rollback();
            m_timepoint.emplace_back( casual::common::platform::time::clock::type::now(), "tx_rollback");

         }
         else
         {
            tx_commit();
            m_timepoint.emplace_back( casual::common::platform::time::clock::type::now(), "tx_commit");
         }
      }
   }
private:
   Settings& m_settings;
   std::vector< Timeoint>& m_timepoint;
};

void run( Settings settings)
{
   for( long iteration = 1; iteration <= settings.iterations; ++iteration)
   {
      std::vector< Timeoint> timepoints;

      {
         timepoints.emplace_back( casual::common::platform::time::clock::type::now(), "start");

         Transaction transaction( settings, timepoints);


         using Send = casual::serviceframework::service::protocol::binary::Send;
         Send send;

         send << CASUAL_MAKE_NVP( settings.argument);

         std::vector< Send::receive_type> receivers;
         receivers.reserve( settings.calls);

         for( long index = 0; index < settings.calls; ++index )
         {
            receivers.push_back( send( settings.service));
         }

         timepoints.emplace_back( casual::common::platform::time::clock::type::now(), "call");


         for( auto& receive : receivers)
         {
            try
            {
               auto result = receive();
            }
            catch( const casual::serviceframework::exception::Base& exception)
            {
               std::cerr << exception << '\n';
            }
            catch( ...)
            {
               casual::common::exception::handle();
            }
         }

         timepoints.emplace_back( casual::common::platform::time::clock::type::now(), "receive");


      }

      std::cout << "time spent for iteration # " << iteration << ":\n";

      auto current = timepoints.begin();
      auto next = current + 1;

      for( ; next !=  std::end( timepoints); ++current, ++next)
      {
         casual::common::log::line( std::cout, std::left, std::setw(10), std::setfill( '.'), next->info, ": ",
               std::right, std::setw( 7), std::setfill( ' '), next->time - current->time);
      }

      casual::common::log::line( std::cout, std::left, std::setw(10), std::setfill( '.'), "total", ": ",
         std::right, std::setw( 7), std::setfill( ' '), timepoints.back().time - timepoints.front().time);
   }

}


int main( int argc, char** argv)
{
   try
   {

      Settings settings;
      {
         using namespace casual::common::argument;
         Parse parse{ "some test crap...",
            Option( std::tie( settings.service), { "-s", "--service"}, "service to call")( cardinality::one{}),
            Option( std::tie( settings.calls), { "-n", "--number"}, "number of async calls to service"),
            Option( std::tie( settings.argument), { "-a", "--argument"}, "argument to the service"),
            Option( std::tie( settings.transaction), { "-t", "--transaction"}, "call within a transaction"),
            Option( std::tie( settings.rollback), { "-r", "--rollback"}, "call within a transaction"),
            Option( std::tie( settings.iterations), { "-i", "--iterations"}, "number of iterations of batch-calls"),
         };
         parse( argc, argv);
      }

      std::cout << "argument: " << settings.argument << '\n';


      run( settings);
   }
   catch( ...)
   {
      casual::common::exception::handle( std::cerr);
   }


}
