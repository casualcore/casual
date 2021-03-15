//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/serialize/native/binary.h"
#include "common/message/type.h"

namespace casual
{
   namespace common::serialize::native
   {
      namespace customization
      {
         //! needs to be specialized for all 'complete message' types (ipc, tcp, stream...)
         template< typename Complete>
         struct point;
         
      } // customization


      template< typename C, typename M>
      C complete( M&& message)
      {
         if( ! message.execution)
            message.execution = execution::id();

         using writer = typename customization::point< std::decay_t< C>>::writer;

         auto archive = writer{}();
         archive << message;

         using casual::common::message::type;
         return C{
            type( message), 
            message.correlation ? message.correlation : uuid::make(),
            archive.consume()};
      }

      template< typename C, typename M>
      void complete( C&& complete, M& message)
      {
         using casual::common::message::type;
         assert( complete.type() == type( message));

         message.correlation = complete.correlation();

         using reader = typename customization::point< std::decay_t< C>>::reader;

         auto archive = reader{}( complete.payload);
         archive >> message;
      }

   } // common::serialize::native
} // casual


