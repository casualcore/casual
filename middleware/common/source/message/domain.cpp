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

            } // configuration


         } // domain

      } // message
   } // common

} // casual
