//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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
               namespace connect
               {
                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ domain: " << value.domain 
                        << ", versions: " << range::make( value.versions)
                        << '}';
                  }

                  std::ostream& operator << ( std::ostream& out, const Reply& value)
                  {
                     return out << "{ domain: " << value.domain 
                        << ", version: " << value.version
                        << '}';
                  }
               } // connect

               namespace discover
               {

                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ process: " << value.process
                           << ", domain: " << value.domain
                           << ", services: " << range::make( value.services)
                           << ", queues: " << range::make( value.queues)
                           << '}';
                  }



                  std::ostream& operator << ( std::ostream& out, const Reply& value)
                  {
                     return out << "{ domain: " << value.domain
                           << ", process: " << value.process
                           << ", services: " << range::make( value.services)
                           << ", queues: " << range::make( value.queues)
                           << '}';
                  }


                  namespace accumulated
                  {

                     std::ostream& operator << ( std::ostream& out, const Reply& value)
                     {
                        return out << "{ replies: " << range::make( value.replies)
                           << '}';
                     }

                  } // accumulated


               } // discover
            } // domain
         } // gateway
      } // message
   } // common
} // casual
