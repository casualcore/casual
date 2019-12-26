//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/communication/message.h"
#include "common/serialize/native/binary.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         namespace native
         {
            template< typename M, typename C = binary::create::Writer>
            communication::message::Complete complete( M&& message, C creator = binary::create::Writer{})
            {
               if( ! message.execution)
                  message.execution = execution::id();

               using casual::common::message::type;
               communication::message::Complete complete( type( message), message.correlation ? message.correlation : uuid::make());

               auto archive = creator( complete.payload);
               archive << message;

               return complete;
            }

            template< typename M, typename C = binary::create::Reader>
            void complete( communication::message::Complete& complete, M& message, C creator = binary::create::Reader{})
            {
               using casual::common::message::type;
               assert( complete.type == type( message));

               message.correlation = complete.correlation;

               auto archive = creator( complete.payload);
               archive >> message;
            }
         } // native
      } // serialize

      namespace communication
      {
         namespace message
         {
            template< typename M>
            communication::message::Complete& operator >> ( communication::message::Complete& complete, M& message)
            {
               assert( complete.type == message.type());

               message.correlation = complete.correlation;

               auto archive = serialize::native::binary::reader( complete.payload);
               archive >> message;

               return complete;
            }
         } // message
      } // communication
   } // common
} // casual


