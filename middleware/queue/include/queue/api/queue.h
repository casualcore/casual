//!
//! casual
//!

#ifndef CASUAL_QUEUE_QUEUE_H_
#define CASUAL_QUEUE_QUEUE_H_


#include "queue/api/message.h"

#include "sf/platform.h"


namespace casual
{
   namespace queue
   {
      inline namespace v1  {

      sf::platform::Uuid enqueue( const std::string& queue, const Message& message);

      std::vector< Message> dequeue( const std::string& queue);
      std::vector< Message> dequeue( const std::string& queue, const Selector& selector);

      namespace blocking
      {
         Message dequeue( const std::string& queue);
         Message dequeue( const std::string& queue, const Selector& selector);
      } // blocking

      namespace peek
      {
         std::vector< message::Information> information( const std::string& queue);
         std::vector< message::Information> information( const std::string& queue, const Selector& selector);

         std::vector< Message> messages( const std::string& queue, const std::vector< queue::Message::id_type>& ids);
      } // peek

      namespace xatmi
      {

         sf::platform::Uuid enqueue( const std::string& queue, const Message& message);

         std::vector< Message> dequeue( const std::string& queue);
         std::vector< Message> dequeue( const std::string& queue, const Selector& selector);

      } // xatmi

      namespace restore
      {
         struct Affected
         {
            std::string queue;
            sf::platform::size::type restored = 0;
         };

         std::vector< Affected> queue( const std::vector< std::string>& queues);


      } // restore

      } // v1
   } // queue
} // casual

#endif // QUEUE_H_
