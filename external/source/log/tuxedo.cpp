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
      namespace local
      {
         namespace time
         {
            namespace
            {
               std::pair<std::string, std::string> divide(const common::platform::time_point& timepoint)
               {
                  auto time_count = std::chrono::time_point_cast<std::chrono::nanoseconds>(timepoint).time_since_epoch().count();
                  std::string text = std::to_string( time_count);
                  return std::make_pair( text.substr(0,10), text.substr(10));
               }
            }
         }
      }

      namespace log
      {
         namespace tuxedo
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
                  auto start = local::time::divide( message.start);
                  auto end = local::time::divide( message.end);
                  m_logfile << "@" << message.service << " ";
                  m_logfile << message.pid << "       ";
                  m_logfile << start.first << "   " << start.second << "    ";
                  m_logfile << end.first << "   " << end.second << "   ";
                  m_logfile << std::endl;
               }

               void persist_commit() override
               {
                  // no-op
               }

            private:
               std::ofstream m_logfile;
            };

         }

      } // log



   } // traffic
} // casual



int main( int argc, char **argv)
{


   // get log-file from arguments
   std::string file{"txrpt.stat"};
   {
      casual::common::Arguments parser;
      parser.add(
            casual::common::argument::directive( { "-f", "--file"}, "path to log-file", file)
      );

      parser.parse( argc, argv);
      casual::common::process::path( parser.processName());
   }

   casual::traffic::log::tuxedo::Handler handler{ file};

   casual::traffic::Receiver receive;
   return receive.start( handler);
}
