//!
//! casual
//!

#include "common/communication/socket.h"

#include "common/uuid.h"
#include "common/string.h"
#include "common/environment.h"

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace domain
         {
            struct Address
            {
               Uuid key;

               std::string name() const 
               { 
                  return string::compose( environment::directory::temporary(), '/', key, ".socket");
               }

               static Address create()
               {
                  Address result;
                  result.key = uuid::make();
                  return result;
               } 
            };

            namespace address
            {
               Address create()
               {
                  Address result;
                  result.key = uuid::make();
                  return result;
               }
            } // address
            
            
            Socket connect( const Address& address);



         } // domain
      
      } // communication
   } // common
} // casual
