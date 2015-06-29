//!
//! type.cpp
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#include "common/message/type.h"

#include "common/process.h"
#include "common/ipc.h"


namespace casual
{

   namespace common
   {
      namespace message
      {

         std::ostream& operator << ( std::ostream& out, const Service& value)
         {
            return out << "{ name: " << value.name << ", type: " << value.type << ", timeout: "
               << value.timeout.count() << ", mode: " << value.transaction << ", traffic: " << range::make( value.traffic_monitors) << '}';
         }

         namespace server
         {



         } // server

      } // message
   } // common
} // casual
