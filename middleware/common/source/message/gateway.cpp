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

                  std::ostream& operator << ( std::ostream& out, const Reply& value)
                  {
                     return out << "{ remote: " << value.remote
                           << ", address: " << range::make( value.address)
                           << ", process: " << value.process
                           << '}';
                  }




               } // discover
            } // domain
         } // gateway
      } // message
   } // common
} // casual
