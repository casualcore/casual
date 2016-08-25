//!
//! casual_broker_main.cpp
//!
//! casual
//!


#include "broker/broker.h"


#include "common/error.h"
#include "common/arguments.h"


#include <iostream>

namespace casual
{
   using namespace common;
   namespace broker
   {
      int main( int argc, char** argv)
      {
         try
         {

            broker::Settings settings;

            {
               Arguments parser{ "casual broker",
                  {
                        argument::directive( { "--forward"}, "path to the forward instance - mainly for unittest", settings.forward)
                  }

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
   } // broker

} // casual


int main( int argc, char** argv)
{
   return casual::broker::main( argc, argv);
}
