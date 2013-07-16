//!
//! transaction_monitor_main.cpp
//!
//! Created on: Jul 15, 2013
//!     Author: Lazan
//!



#include "common/error.h"

#include "transaction/manager.h"






int main( int argc, char** argv)
{

   try
   {
      std::vector< std::string> arguments;

      std::copy(
         argv,
         argv + argc,
         std::back_inserter( arguments));

      casual::transaction::Manager manager( arguments);

      manager.start();

   }
   catch( ...)
   {
      return casual::common::error::handler();

   }
   return 0;
}

