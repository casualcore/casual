/*
 * server.cpp
 *
 *  Created on: 22 apr 2015
 *      Author: 40043280
 */


#include "common/message/server.h"

namespace casual
{
   namespace common
   {
      namespace message
      {
         namespace service
         {

            namespace call
            {

               std::ostream& operator << ( std::ostream& out, const base_call& value)
               {
                  return out << "{ process: " << value.process
                     << ", service: " << value.service
                     << ", parent: " << value.parent
                     << ", flags: " << value.flags
                     << '}';
               }

               namespace callee
               {
                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ base: " << static_cast< const base_call&>( value) << ", buffer: " << value.buffer << '}';
                  }

               } // caller

            } // call
         }
      } // message
   } // common
} // casual
