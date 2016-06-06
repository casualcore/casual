//!
//! casual 
//!

#include "common/message/domain.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace domain
         {
            namespace scale
            {

               std::ostream& operator << ( std::ostream& out, const Executable& value)
               {
                  return out << "{ executables: " << range::make( value.executables)
                        << '}';
               }

            } // scale
            namespace process
            {

               namespace lookup
               {
                  std::ostream& operator << ( std::ostream& out, Request::Directive value)
                  {
                     if( value == Request::Directive::wait) { return out << "wait";}
                     return out << "direct";
                  }

                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ identification: " << value.identification
                           << ", pid: " << value.pid
                           << ", directive: " << value.directive
                           << ", process: " << value.process
                           << '}';
                  }

               } // lookup

            } // process

            namespace configuration
            {
               namespace transaction
               {
                  std::ostream& operator << ( std::ostream& out, const Resource& value)
                  {
                     return out << "{ id: " << value.id
                           << ", key: " << value.key
                           << ", openinfo: " << value.openinfo
                           << ", closeinfo: " << value.closeinfo
                           << ", instances: " << value.instances
                           << '}';
                  }

                  namespace resource
                  {

                     std::ostream& operator << ( std::ostream& out, const Reply& value)
                     {
                        return out << "{ process: " << value.process
                              << ", resources: " << range::make( value.resources)
                              << '}';
                     }
                  } // resource

               } // transaction

               namespace gateway
               {

                  std::ostream& operator << ( std::ostream& out, const Listener& value)
                  {
                     return out << "{ adress: " << value.address
                           << '}';
                  }

                  std::ostream& operator << ( std::ostream& out, Connection::Type value)
                  {
                     switch( value)
                     {
                        case Connection::Type::ipc: return out << "ipc";
                        case Connection::Type::tcp: return out << "tcp";
                        default: return out << "unknown";
                     }

                  }

                  std::ostream& operator << ( std::ostream& out, const Connection& value)
                  {
                     return out << "{ name: " << value.name
                           << ", address: " << value.address
                           << ", type: " << value.type
                           << ", restart: " << value.restart
                           << ", services: " << range::make( value.services)
                           << '}';
                  }

                  std::ostream& operator << ( std::ostream& out, const Reply& value)
                  {
                     return out << "{ listeners: " << range::make( value.listeners)
                           << ", connections: " << range::make( value.connections)
                           << '}';
                  }
               } // gateway

            } // configuration


         } // domain

      } // message
   } // common

} // casual
