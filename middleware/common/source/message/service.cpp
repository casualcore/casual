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
                  << ", category: " << value.category
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


            std::ostream& operator << ( std::ostream& out, Advertise::Directive value)
            {
               switch( value)
               {
                  case Advertise::Directive::add: return out << "add";
                  case Advertise::Directive::remove: return out << "remove";
                  case Advertise::Directive::replace: return out << "replace";
               }
               return out << "unknown";
            }

            std::ostream& operator << ( std::ostream& out, const Advertise& message)
            {
               return out << "{ process: " << message.process
                     << ", directive: " << message.directive
                     << ", services: " << range::make( message.services)
                     << '}';
            }

            namespace lookup
            {
               std::ostream& operator << ( std::ostream& out, const Request::Context& value)
               {
                  switch( value)
                  {
                     case Request::Context::forward: return out << "forward";
                     case Request::Context::gateway: return out << "gateway";
                     case Request::Context::no_reply: return out << "no_reply";
                     case Request::Context::regular: return out << "regular";
                  }
                  return out << "unknown";
               }

               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ process: " << value.process
                        << ", requested: " << value.requested
                        << ", context: " << value.context
                        << '}';

               }

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
