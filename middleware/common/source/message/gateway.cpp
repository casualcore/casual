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
               namespace service
               {
                  namespace advertise
                  {
                     std::ostream& operator << ( std::ostream& out, const Service& message)
                     {
                        return out << "{ name: " << message.name
                              << ", type: " << message.type
                              << ", transaction: " << message.transaction
                              << ", hops: " << message.hops
                              << '}';
                     }

                  } // advertise

                  std::ostream& operator << ( std::ostream& out, const Advertise& message)
                  {
                     return out << "{ process: " << message.process
                           << ", domain: " << message.domain
                           << ", order: " << message.order
                           << ", services: " << range::make( message.services)
                           << '}';
                  }

                  std::ostream& operator << ( std::ostream& out, const Unadvertise& message)
                  {
                     return out << "{ process: " << message.process
                           << ", services: " << range::make( message.services)
                           << '}';
                  }

               } // service
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
                     return out << "{ domain: " << value.domain
                           << ", process: " << value.process
                           << ", services: " << range::make( value.services)
                           << '}';
                  }




               } // discover
            } // domain
         } // gateway
      } // message
   } // common
} // casual
