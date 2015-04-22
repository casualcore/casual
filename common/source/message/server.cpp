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
                  return out << "caller: " << value.caller << ", service: " << value.service;
               }

               namespace callee
               {
                  std::ostream& operator << ( std::ostream& out, const Request& value)
                  {
                     return out << "{ " << static_cast< const base_call&>( value) << ", buffer: " << value.buffer << '}';
                  }

               } // caller

            } // call
         }
      } // message
   } // common
} // casual
