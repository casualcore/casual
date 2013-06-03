//!
//! action.cpp
//!
//! Created on: May 12, 2013
//!     Author: Lazan
//!

#include "broker/action.h"

#include "common/process.h"

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
               common::process::spawn( server.path, {});
            }

         } // server
      } // action
   } // broker
} // casual


