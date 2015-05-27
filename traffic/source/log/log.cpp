//!
//! log.cpp
//!
//! Created on: May 6, 2015
//!     Author: Lazan
//!

#include "receiver.h"

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


            void log( const message_type& message) override
            {
               m_logfile << message.service
                     << "|" << message.parentService
                     << "|" << message.pid
                     << "|" << message.callId
                     << "|" << message.transactionId
                     << "|" << std::chrono::duration_cast< std::chrono::microseconds>( message.start.time_since_epoch()).count()
                     << "|" << std::chrono::duration_cast< std::chrono::microseconds>( message.end.time_since_epoch()).count() << std::endl;
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
      casual::common::Arguments parser;
      parser.add(
            casual::common::argument::directive( { "-f", "--file"}, "path to log-file", file)
      );

      parser.parse( argc, argv);
      casual::common::process::path( parser.processName());
   }

   casual::traffic::log::Handler handler{ file};

   casual::traffic::Receiver receive;
   return receive.start( handler);
}
