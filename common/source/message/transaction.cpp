//!
//! transaction.cpp
//!
//! Created on: May 31, 2015
//!     Author: Lazan
//!

#include "common/message/transaction.h"


namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace transaction
         {
            namespace resource
            {

               std::ostream& operator << ( std::ostream& out, const Involved& value)
               {
                  return out << "{ process: " << value.process
                        << ", resources: " << range::make( value.resources)
                        << ", trid: " << value.trid
                        << '}';
               }



               namespace connect
               {

                  std::ostream& operator << ( std::ostream& out, const Reply& message)
                  {
                     return out << "{ process: " << message.process << ", resource: " << message.resource << ", state: " << message.state << "}";
                  }

               } // connect

            } // resource
         } // transaction
      } // message
   } // common
} // casual

