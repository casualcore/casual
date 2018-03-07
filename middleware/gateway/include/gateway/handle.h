//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_HANDLE_H_
#define CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_HANDLE_H_

#include "gateway/message.h"


#include <thread>

namespace casual
{
   namespace gateway
   {
      namespace handle
      {
         struct Disconnect
         {
            using message_type = message::worker::Disconnect;

            Disconnect( std::thread& thread);

            void operator() ( message_type& message);


         private:
            std::thread& m_thread;
         };

      } // handle
   } // gateway


} // casual

#endif // CASUAL_MIDDLEWARE_GATEWAY_INCLUDE_GATEWAY_HANDLE_H_
