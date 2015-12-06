//!
//! log.cpp
//!
//! Created on: May 6, 2015
//!     Author: Lazan
//!

#include "traffic/receiver.h"

#include "common/internal/trace.h"
#include "common/arguments.h"
#include "common/process.h"

//
// std
//
#include <fstream>
#include <iostream>

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

            void persist_begin( ) override
            {
               // no-op
            }


            void log( const event_type& event) override
            {
               m_logfile << event.service()
                     << "|" << event.parent()
                     << "|" << event.pid()
                     << "|" << event.execution()
                     << "|" << event.transaction()
                     << "|" << std::chrono::duration_cast< std::chrono::microseconds>( event.start().time_since_epoch()).count()
                     << "|" << std::chrono::duration_cast< std::chrono::microseconds>( event.end().time_since_epoch()).count()
                     << '\n';
            }

            void persist_commit() override
            {
               // no-op
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
   std::string file{"statistics.log"};
   {
      casual::common::Arguments parser{
         { casual::common::argument::directive( { "-f", "--file"}, "path to log-file", file)}
      };

      parser.parse( argc, argv);
   }

   casual::traffic::log::Handler handler{ file};

   casual::traffic::Receiver receive;
   return receive.start( handler);
}
