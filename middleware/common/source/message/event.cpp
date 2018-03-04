//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/message/event.h"


namespace casual
{

   namespace common
   {
      namespace message
      {
         namespace event
         {
            namespace subscription
            {
               inline namespace v1
               {

                  std::ostream& operator << ( std::ostream& out, const Begin& value)
                  {
                     return out << "{ process: " << value.process
                           << ", types: " << range::make( value.types)
                           << '}';
                  }


                  std::ostream& operator << ( std::ostream& out, const End& value)
                  {
                     return out << "{ process: " << value.process
                           << '}';
                  }

               }
            } // subscription

            namespace domain
            {
               inline namespace v1
               {
                  std::ostream& operator << ( std::ostream& out, Error::Severity value)
                  {
                     switch( value)
                     {
                        case Error::Severity::warning: return out << "warning";
                        case Error::Severity::error: return out << "error";
                        case Error::Severity::fatal: return out << "fatal";
                     }
                     return out << "unknown";
                  }
                  std::ostream& operator << ( std::ostream& out, const Error& value)
                  {
                     return out << "{ severity: " << value.severity
                           << ", message: " << value.message
                           << ", executable: " << value.executable
                           << ", pid: " << value.pid
                           << '}';
                  }

               }
            }

            namespace process
            {
               inline namespace v1
               {
                  std::ostream& operator << ( std::ostream& out, const Spawn& value)
                  {
                     return out << "{ path: " << value.path
                           << ", pids: " << range::make( value.pids)
                           << '}';
                  }

                  std::ostream& operator << ( std::ostream& out, const Exit& value)
                  {
                     return out << "{ state: " << value.state
                           << '}';
                  }
               }

            } // process

            namespace service
            {
               inline namespace v1
               {
                  std::ostream& operator << ( std::ostream& out, const Call& value)
                  {
                     return out << "{ process: " << value.process
                           << ", service: " << value.service
                           << ", parent: " << value.parent
                           << ", start: " << std::chrono::duration_cast< common::platform::time::unit>( value.start.time_since_epoch()).count()
                           << ", end: " << std::chrono::duration_cast< common::platform::time::unit>( value.end.time_since_epoch()).count()
                           << '}';
                  }
               }
            } // service

         } // event
      } // message
   } // common
} // casual
