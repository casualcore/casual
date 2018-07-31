//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/api/message.h"

#include "common/chronology.h"


namespace casual
{
   namespace queue
   {
      inline namespace v1  
      {
         std::ostream& operator << ( std::ostream& out, const Attributes& value)
         {
            return out << "{ properties: " << value.properties
               << ", reply: " << value.reply
               << ", available: " << common::chronology::local( value.available)
               << '}';
         }

         std::ostream& operator << ( std::ostream& out, const Payload& value)
         {
            return out << "{ type: " << value.type
               << ", data: " << value.data.data()
               << '}';
         }

      } // v1
   } // queue

} // casual


