//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MARSHAL_COMPLETE_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MARSHAL_COMPLETE_H_

#include "common/communication/message.h"
#include "common/marshal/binary.h"

namespace casual
{
   namespace common
   {

      namespace marshal
      {
         template< typename M, typename C = binary::create::Output>
         communication::message::Complete complete( M&& message, C creator = binary::create::Output{})
         {
            if( ! message.execution)
            {
               message.execution = execution::id();
            }

            communication::message::Complete complete( message.type(), message.correlation ? message.correlation : uuid::make());

            auto marshal = creator( complete.payload);
            marshal << message;

            return complete;
         }

         template< typename M, typename C = binary::create::Input>
         void complete( communication::message::Complete& complete, M& message, C creator = binary::create::Input{})
         {
            assert( complete.type == message.type());

            message.correlation = complete.correlation;

            auto marshal = creator( complete.payload);
            marshal >> message;
         }

      } // marshal

   } // common
} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_MARSHAL_COMPLETE_H_
