//!
//! log.cpp
//!
//! Created on: May 6, 2015
//!     Author: Lazan
//!

#include "proposal/receiver.h"


#include "common/internal/trace.h"

//
// std
//
#include <fstream>


//
// example of traffic log implementation.
// we can keep this in one source file, no need
// to split it into several.
//


namespace casual
{
   namespace traffic
   {
      namespace log
      {
         struct Handler : handler::Base
         {
            Handler( const std::string& file) : m_logfile{ file}
            {

            }

            void log( const message_type& message) override
            {
               // write to logfile...
            }

            void persist() override
            {
               // no-op?
            }

         private:
            std::ofstream m_logfile;
         };

      } // log



   } // traffic
} // casual



int main( int argc, char **argv)
{


   // get log-file from arguments
   std::string file;

   // set process name from arguments



   casual::traffic::log::Handler handler{ file};

   casual::traffic::Receiver receive;
   return receive.start( handler);
}
