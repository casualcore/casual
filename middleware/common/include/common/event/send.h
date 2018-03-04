//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_EVENT_SEND_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_EVENT_SEND_H_


#include "common/message/event.h"

#include <string>

namespace casual
{
   namespace common
   {
      namespace event
      {

         namespace error
         {
            using Severity = message::event::domain::Error::Severity;

            //!
            //! Sends an error event to the domain manager, that will forward the event
            //! to possible listeners
            //!
            //! @param message
            //!
            void send( std::string message, Severity severity = Severity::error);

         } // error


      } // event
   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_EVENT_SEND_H_
