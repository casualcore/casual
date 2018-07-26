//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "gateway/message.h"

namespace casual
{
   namespace gateway
   {
      namespace message
      {
         namespace outbound
         {
            namespace configuration
            {


               std::ostream& operator << ( std::ostream& out, const Request& value)
               {
                  return out << "{ process: " << value.process
                        << '}';
               }


               std::ostream& operator << ( std::ostream& out, const Reply& value)
               {
                  return out << "{ process: " << value.process
                        << ", services: " << common::range::make( value.services)
                        << ", queues: " << common::range::make( value.queues)
                        << '}';
               }

            } // configuration
         } // outbound


         namespace inbound
         {
            std::ostream& operator << ( std::ostream& out, const Limit& value)
            {
               return out << "{ size: " << value.size
                  << ", messages: " << value.messages
                  << '}';
            }
         } // inbound

         namespace outbound
         {
            namespace connect 
            {
               std::ostream& operator << ( std::ostream& out, const Done& value)
               {
                  return out << "{ descriptor: " << value.descriptor << '}';
               }
            } // connect 
         } // outbound

      } // message

   } // gateway


} // casual
