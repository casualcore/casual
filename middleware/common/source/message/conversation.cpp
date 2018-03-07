//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
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

            namespace connect
            {
               std::ostream& operator << ( std::ostream& out, const basic_request& value)
               {
                  return out << "{ process: " << value.process
                        << ", service: " << value.service
                        << ", parent: " << value.parent
                        << ", trid: " << value.trid
                        << ", header: " << range::make( value.header)
                        << ", flags: " << value.flags
                        << ", recording: " << value.recording
                        << '}';
               }

               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  return out << "{ process: " << value.process
                        << ", route: " << value.route
                        << ", recording: " << value.recording
                        << ", status: " << value.status
                        << '}';
               }

            } // connect

            std::ostream& operator << ( std::ostream& out, const basic_send& value)
            {
               return out << "{ " << value.correlation
                     << ", process: " << value.process
                     << ", flags: " << value.flags
                     << ", events: " << value.events
                     << ", route: " << value.route
                     << '}';
            }


            std::ostream& operator << ( std::ostream& out, const Disconnect& value)
            {
               return out << "{ " << value.correlation
                     << ", process: " << value.process
                     << ", events: " << value.events
                     << ", route: " << value.route
                     << '}';
            }

         } // conversation


      } // message
   } // common
} // casual

