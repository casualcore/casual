//!
//! action.cpp
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#include "broker/action.h"

#include "common/process.h"
#include "common/string.h"

namespace casual
{
   namespace broker
   {
      namespace action
      {
         namespace server
         {

            std::vector< common::platform::pid_type> start( const configuration::Server& server)
            {
               std::vector< common::platform::pid_type> result;

               for( auto count = std::stol( server.instances); count > 0; --count)
               {
                  auto pid = common::process::spawn( server.path, common::string::split( server.arguments));
                  result.push_back( pid);
               }
               return result;
            }

         } // server
      } // action
   } // broker
} // casual


