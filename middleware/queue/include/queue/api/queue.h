//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_QUEUE_QUEUE_H_
#define CASUAL_QUEUE_QUEUE_H_


#include "queue/api/message.h"

#include "serviceframework/platform.h"


namespace casual
{
   namespace queue
   {
      inline namespace v1  {

      serviceframework::platform::Uuid enqueue( const std::string& queue, const Message& message);

      std::vector< Message> dequeue( const std::string& queue);
      std::vector< Message> dequeue( const std::string& queue, const Selector& selector);

      namespace blocking
      {
         Message dequeue( const std::string& queue);
         Message dequeue( const std::string& queue, const Selector& selector);

         namespace available
         {
            //!
            //! If requested queue is not found (advertised), we will wait until it's
            //! available, and then block.
            //!
            //! @{
            Message dequeue( const std::string& queue);
            Message dequeue( const std::string& queue, const Selector& selector);
            //! @}

         } // available
      } // blocking

      namespace peek
      {
         std::vector< message::Information> information( const std::string& queue);
         std::vector< message::Information> information( const std::string& queue, const Selector& selector);

         std::vector< Message> messages( const std::string& queue, const std::vector< queue::Message::id_type>& ids);
      } // peek

      namespace xatmi
      {

         serviceframework::platform::Uuid enqueue( const std::string& queue, const Message& message);

         std::vector< Message> dequeue( const std::string& queue);
         std::vector< Message> dequeue( const std::string& queue, const Selector& selector);

      } // xatmi

      namespace restore
      {
         struct Affected
         {
            std::string queue;
            serviceframework::platform::size::type restored = 0;
         };

         std::vector< Affected> queue( const std::vector< std::string>& queues);


      } // restore

      } // v1
   } // queue
} // casual

#endif // QUEUE_H_
