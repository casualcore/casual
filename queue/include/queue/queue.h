//!
//! queue.h
//!
//! Created on: Jul 9, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_H_
#define CASUAL_QUEUE_H_

#include "common/uuid.h"
#include "common/platform.h"
#include "common/move.h"

namespace casual
{
   namespace queue
   {
      struct Attributes
      {
         //!
         //! Correlation information.
         //!
         std::string correlation;

         //!
         //! reply queue.
         //!
         std::string reply;

         //!
         //! When the message is available, in absolute time.
         //!
         common::platform::time_type available;

      };

      struct Payload
      {
         std::size_t type = 0;
         common::platform::binary_type data;
      };

      struct Message
      {
         //!
         //! Correlation information.
         //!
         std::string correlation;

         //!
         //! reply queue.
         //!
         std::string reply;

         //!
         //! When the message is available, in absolute time.
         //!
         common::platform::time_type available;
      };


      namespace xatmi
      {

         struct Message
         {
            Message();
            ~Message();

            const common::Uuid& id() const;

            const std::string& correlation() const;
            Message& correlation( std::string value);

            const std::string& reply() const;
            Message& reply( std::string value);

            common::platform::time_type available() const;
            Message& available( common::platform::time_type value);

            common::platform::raw_buffer_type payload() const;
            Message& payload( common::platform::raw_buffer_type value);


         private:
            class Implementation;
            common::move::basic_pimpl< Implementation> m_implementation;
         };

      } // xatmi




      common::Uuid enqueue( const std::string& queue, const Message& message);


      common::Uuid enqueue( const std::string& queue, const Attributes& attributes);


      std::vector< Message> dequeue( const std::string& queue);



   } // queue
} // casual

#endif // QUEUE_H_
