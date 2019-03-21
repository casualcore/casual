//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "retry/send/message.h"
#include "retry/send/common.h"

#include "common/communication/instance.h"

namespace casual
{
   using namespace common;
   namespace retry
   {
      namespace send
      {
         namespace message
         {
            namespace local
            {
               namespace
               {
                  auto& device() 
                  {
                     static communication::instance::outbound::detail::Device device{ send::identification, send::environment}; 
                     return device;  
                  }
                  
               } // <unnamed>
            } // local
            std::ostream& operator << ( std::ostream& out, const Request& rhs)
            {
               return out << "{ destination: " << rhs.destination
                  << ", message: " << rhs.message
                  << '}';
            }

            void send( const Request& request)
            {
               Trace trace{ "retry::send::message::send"};

               communication::ipc::blocking::send( local::device(), request);
            }
         } // message         

      } // send
      
   } // retry
} // casual