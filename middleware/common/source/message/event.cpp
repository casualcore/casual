//!
//! casual 
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
                           << ", start: " << std::chrono::duration_cast< std::chrono::microseconds>( value.start.time_since_epoch()).count()
                           << ", end: " << std::chrono::duration_cast< std::chrono::microseconds>( value.end.time_since_epoch()).count()
                           << '}';
                  }
               }
            } // service

         } // event
      } // message
   } // common
} // casual
