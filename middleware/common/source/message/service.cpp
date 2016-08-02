//!
//! casual
//!

#include "common/message/service.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         std::ostream& operator << ( std::ostream& out, const Service& value)
         {
            return out << "{ name: " << value.name
                  << ", type: " << value.type
                  << ", timeout: " << value.timeout.count()
                  << ", mode: " << value.transaction
                  << '}';
         }

         namespace service
         {

            std::ostream& operator << ( std::ostream& out, const Transaction& message)
            {
               return out << "{ trid: " << message.trid
                     << ", state: " << message.state
                     << '}';
            }

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
                     << ", services: " << range::make( message.services)
                     << '}';
            }


            namespace lookup
            {

               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  out << "{ service: " << value.service << ", process: " << value.process << ", state: ";
                  switch( value.state)
                  {
                     case Reply::State::absent: out << "absent"; break;
                     case Reply::State::idle: out << "idle"; break;
                     case Reply::State::busy: out << "busy"; break;
                  }

                  return out << '}';
               }

            } // lookup

            namespace call
            {

               std::ostream& operator << ( std::ostream& out, const base_call& value)
               {
                  auto& header = common::service::header::fields();

                  return out << "{ process: " << value.process
                     << ", service: " << value.service
                     << ", parent: " << value.parent
                     << ", flags: " << value.flags
                     << ", header: " << range::make( header)
                     << '}';
               }

               namespace callee
               {
                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ base: " << static_cast< const base_call&>( value) << ", buffer: " << value.buffer << '}';
                  }

               } // caller

               std::ostream& operator << ( std::ostream& out, const Reply& message)
               {
                  return out << "{ descriptor: " << message.descriptor
                        << ", transaction: " << message.transaction
                        << ", error: " << message.error
                        << ", code: " << message.code
                        << ", buffer: " << message.buffer
                        << '}';
               }

            } // call
         }
      } // message
   } // common
} // casual
