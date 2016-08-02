//!
//! casual 
//!

#include "common/message/gateway.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace gateway
         {
            namespace domain
            {
               namespace discover
               {

                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ process: " << value.process
                           << ", domain: " << value.domain
                           << ", services: " << range::make( value.services)
                           << '}';
                  }



                  std::ostream& operator << ( std::ostream& out, const Reply& value)
                  {
                     return out << "{ remote: " << value.remote
                           << ", process: " << value.process
                           << ", services: " << range::make( value.services)
                           << ", address: " << range::make( value.address)

                           << '}';
                  }




               } // discover
            } // domain
         } // gateway
      } // message
   } // common
} // casual
