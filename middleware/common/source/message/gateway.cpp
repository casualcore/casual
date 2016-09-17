//!
//! casual 
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

                  std::ostream& operator << ( std::ostream& out, const Queue& message)
                  {
                     return out << "{ name: " << message.name
                           << '}';
                  }
               } // advertise

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
                        << ", domain: " << message.domain
                        << ", directive: " << message.directive
                        << ", order: " << message.order
                        << ", services: " << range::make( message.services)
                        << ", queues: " << range::make( message.queues)
                        << '}';
               }

               namespace discover
               {
                  namespace internal
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
                  } // internal

                  namespace external
                  {
                     std::ostream& operator << ( std::ostream& out, const Request& value)
                     {
                        return out << "{ process: " << value.process
                              << ", domain: " << value.domain
                              << ", services: " << range::make( value.services)
                              << '}';
                     }

                     std::ostream& operator << ( std::ostream& out, const Reply& value)
                     {
                        return out << "{ replies: " << range::make( value.replies)
                           << '}';
                     }

                  } // external


               } // discover
            } // domain
         } // gateway
      } // message
   } // common
} // casual
