//!
//! casual
//!

#include "common/message/type.h"
#include "common/chronology.h"




namespace casual
{

   namespace common
   {
      namespace message
      {



         std::ostream& operator << ( std::ostream& out, const Statistics& message)
         {
            return out << "{ start: " << std::chrono::duration_cast< common::platform::time::unit>( message.start.time_since_epoch()).count()
                  << chronology::unit::string( common::platform::time::unit{})
                  << ", end: " << std::chrono::duration_cast< common::platform::time::unit>( message.end.time_since_epoch()).count()
                  << chronology::unit::string( common::platform::time::unit{})
                  << '}';
         }

         namespace server
         {



         } // server

      } // message
   } // common
} // casual
