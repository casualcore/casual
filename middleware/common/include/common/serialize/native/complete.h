//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/serialize/native/binary.h"
#include "common/strong/type.h"
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
      auto complete( M&& message) -> decltype( void( typename customization::point< std::decay_t< C>>::writer{}() << message), C{})
      {
         if( ! message.execution)
            message.execution = execution::id();

         using writer = typename customization::point< std::decay_t< C>>::writer;

         auto archive = writer{}();
         archive << message;

         return C{
            casual::common::message::type( message), 
            message.correlation ? message.correlation : strong::correlation::id{ uuid::make()},
            archive.consume()};
      }

      template< typename C, typename M>
      auto complete( C&& complete, M& message) -> decltype( void( typename customization::point< std::decay_t< C>>::reader{}( complete.payload) >> message))
      {
         using casual::common::message::type;
         assert( complete.type() == type( message));

         message.correlation = complete.correlation();

         using reader = typename customization::point< std::decay_t< C>>::reader;

         auto archive = reader{}( complete.payload);
         archive >> message;
      }

      template< typename M, typename C>
      auto complete( C& complete) -> decltype( void( native::complete( complete, std::declval< M&>())), M{})
      {
         M message;
         native::complete( complete, message);
         return message;
      }

   } // common::serialize::native
} // casual


