//!
//! queue.cpp
//!
//! Created on: Nov 23, 2014
//!     Author: Lazan
//!

#include "queue/api/queue.h"


#include "sf/xatmi_call.h"


namespace casual
{
   namespace queue
   {

      sf::platform::Uuid enqueue( const std::string& queue, const Message& message)
      {
         sf::xatmi::service::binary::Sync service{ "casual.enqueue"};
         service << CASUAL_MAKE_NVP( queue);
         service << CASUAL_MAKE_NVP( message);

         auto reply = service();

         sf::platform::Uuid returnValue;

         reply >> CASUAL_MAKE_NVP( returnValue);

         return returnValue;
      }

      std::vector< Message> dequeue( const std::string& queue)
      {
         sf::xatmi::service::binary::Sync service{ "casual.dequeue"};
         service << CASUAL_MAKE_NVP( queue);

         auto reply = service();

         std::vector< Message> returnValue;

         reply >> CASUAL_MAKE_NVP( returnValue);

         return returnValue;

      }

      namespace peek
      {
         std::vector< Message> queue( const std::string& queue)
         {
            std::vector< Message> result;

            return result;
         }

      } // peek

   } // queue
} // casual
