//!
//! casual 
//!

#include "common/message/conversation.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace conversation
         {

            namespace local
            {
               namespace
               {
                  template< typename T>
                  std::ostream& stream_request( std::ostream& out, const T& value)
                  {
                     return out << "{ " << value.correlation
                           << ", process: " << value.process
                           << ", flags: " << value.flags
                           << ", events: " << value.events
                           << ", route: " << value.route
                           << ", buffer: " << value.buffer
                           << '}';
                  }

               } // <unnamed>
            } // local

            std::ostream& operator << ( std::ostream& out, const Node& value)
            {
               return out << "{ address: " << value.address << '}';
            }

            std::ostream& operator << ( std::ostream& out, const Route& value)
            {
               return out << "{ nodes: " << range::make( value.nodes) << '}';
            }

            namespace send
            {
               std::ostream& operator << ( std::ostream& out, const base_request& value)
               {
                  return out << "{ " << value.correlation
                        << ", process: " << value.process
                        << ", flags: " << value.flags
                        << ", events: " << value.events
                        << ", route: " << value.route
                        << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  return out << "{ " << value.correlation
                        << ", process: " << value.process
                        << ", events: " << value.events
                        << ", route: " << value.route
                        << '}';
               }
            } // send
         } // conversation


      } // message
   } // common
} // casual

