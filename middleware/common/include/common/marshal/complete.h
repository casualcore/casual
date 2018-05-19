//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


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

            using casual::common::message::type;
            communication::message::Complete complete( type( message), message.correlation ? message.correlation : uuid::make());

            auto marshal = creator( complete.payload);
            marshal << message;

            return complete;
         }

         template< typename M, typename C = binary::create::Input>
         void complete( communication::message::Complete& complete, M& message, C creator = binary::create::Input{})
         {
            using casual::common::message::type;
            assert( complete.type == type( message));

            message.correlation = complete.correlation;

            auto marshal = creator( complete.payload);
            marshal >> message;
         }

      } // marshal

   } // common
} // casual


