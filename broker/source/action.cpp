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

            void start( const configuration::Server& server)
            {
               for( auto count = std::stol( server.instances); count > 0; --count)
               {
                  common::process::spawn( server.path, common::string::split( server.arguments));
               }
            }

         } // server
      } // action
   } // broker
} // casual


