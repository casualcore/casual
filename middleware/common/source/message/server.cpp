//!
//! server.cpp
//!
//! Created on: Jan 7, 2016
//!     Author: Lazan
//!

#include "common/message/server.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace server
         {

            namespace connect
            {

               std::ostream& operator << ( std::ostream& out, const Request& rhs)
               {
                  return out << "{ identification: " << rhs.identification
                        << ", path: " << rhs.path
                        << ", process: " << rhs.process
                        << ", services: " << range::make( rhs.services) << "}\n";
               }

            } // connect

         } // server

      } // message
   } //common
} // casual
