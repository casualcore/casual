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
               namespace connect
               {
                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ identification: " << value.identification
                           << ", process: " << value.process
                           << '}';
                  }
               } // connect
               namespace termination
               {
                  std::ostream& operator << ( std::ostream& out, const Event& value)
                  {
                     return out << "{ death: " << value.death << '}';
                  }
               } // termination

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

         } // domain

      } // message
   } // common

} // casual
