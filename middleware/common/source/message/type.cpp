//!
//! type.cpp
//!
//! Created on: Jun 14, 2014
//!     Author: Lazan
//!

#include "common/message/type.h"




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


         std::ostream& operator << ( std::ostream& out, const Statistics& message)
         {
            return out << "{ start: " << std::chrono::duration_cast< std::chrono::microseconds>( message.start.time_since_epoch()).count()
                  << "us, end: " << std::chrono::duration_cast< std::chrono::microseconds>( message.end.time_since_epoch()).count()
                  << '}';
         }

         namespace server
         {



         } // server

      } // message
   } // common
} // casual
