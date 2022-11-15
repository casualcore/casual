//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "queue/api/message.h"

#include "casual/platform.h"
#include "common/uuid.h"
#include "common/transaction/global.h"
#include "common/functional.h"


namespace casual
{
   namespace queue
   {
      inline namespace v1  {

      common::Uuid enqueue( const std::string& queue, const Message& message);

      [[nodiscard]] std::vector< Message> dequeue( const std::string& queue);
      [[nodiscard]] std::vector< Message> dequeue( const std::string& queue, const Selector& selector);

      namespace blocking
      {
         [[nodiscard]] Message dequeue( const std::string& queue);
         [[nodiscard]] Message dequeue( const std::string& queue, const Selector& selector);

         namespace available
         {
            //! If requested queue is not found (advertised), we will wait until it's
            //! available, and then block.
            //! @{
            [[nodiscard]] Message dequeue( const std::string& queue);
            [[nodiscard]] Message dequeue( const std::string& queue, const Selector& selector);
            //! @}

         } // available
      } // blocking

      namespace peek
      {
         [[nodiscard]] std::vector< message::Information> information( const std::string& queue);
         [[nodiscard]] std::vector< message::Information> information( const std::string& queue, const Selector& selector);

         [[nodiscard]] std::vector< Message> messages( const std::string& queue, const std::vector< queue::Message::id_type>& ids);
      } // peek

      namespace browse
      {
         // we could add _browse::dequeue_ 

         //! peeks the next messages until queue is empty or user return false from `callback`
         void peek( std::string queue, common::unique_function< bool(Message&&)> callback);
         
      } // browse

      
      namespace xatmi
      {
         common::Uuid enqueue( const std::string& queue, const Message& message);

         [[nodiscard]] std::vector< Message> dequeue( const std::string& queue);
         [[nodiscard]] std::vector< Message> dequeue( const std::string& queue, const Selector& selector);

      } // xatmi

      struct Affected
      {
         std::string queue;
         platform::size::type count = 0;
      };

      namespace restore
      {
         using Affected = queue::Affected;
         std::vector< Affected> queue( const std::vector< std::string>& queues);

      } // restore

      namespace clear
      {
         using Affected = queue::Affected;
         std::vector< Affected> queue( const std::vector< std::string>& queues);
      } // clear

      namespace messages
      {
         std::vector< common::Uuid> remove( const std::string& queue, const std::vector< common::Uuid>& messages);
      } // messages

      } // v1
   } // queue
} // casual


