//!
//! queue.h
//!
//! Created on: Jul 9, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_QUEUE_H_
#define CASUAL_QUEUE_QUEUE_H_

#include "common/uuid.h"
#include "common/platform.h"
#include "common/move.h"

#include "sf/namevaluepair.h"

namespace casual
{
   namespace queue
   {
      struct Attributes
      {
         //!
         //! Correlation information.
         //!
         std::string properties;

         //!
         //! reply queue.
         //!
         std::string reply;

         //!
         //! When the message is available, in absolute time.
         //!
         common::platform::time_type available;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            archive & CASUAL_MAKE_NVP( properties);
            archive & CASUAL_MAKE_NVP( reply);
            archive & CASUAL_MAKE_NVP( available);
         })

      };

      struct Payload
      {
         std::size_t type = 0;
         common::platform::binary_type data;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            archive & CASUAL_MAKE_NVP( type);
            archive & CASUAL_MAKE_NVP( data);
         })
      };

      struct Message
      {
         Attributes attribues;
         Payload payload;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            archive & CASUAL_MAKE_NVP( attribues);
            archive & CASUAL_MAKE_NVP( payload);
         })
      };


      namespace xatmi
      {
         struct Paylaod
         {
            common::platform::raw_buffer_type buffer;
            std::size_t size;
         };

         struct Message
         {
            Attributes attribues;
            Payload payload;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( attribues);
               archive & CASUAL_MAKE_NVP( payload);
            })
         };


         /*
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
            struct Implementation;
            common::move::basic_pimpl< Implementation> m_implementation;
         };
         */

      } // xatmi




      common::Uuid enqueue( const std::string& queue, const Message& message);


      std::vector< Message> dequeue( const std::string& queue);


      namespace peek
      {
         struct Message
         {
            common::Uuid id;
            std::size_t type;
            std::size_t state;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( id);
               archive & CASUAL_MAKE_NVP( type);
               archive & CASUAL_MAKE_NVP( state);
            })
         };

         std::vector< Message> queue( const std::string& queue);



      } // peek



   } // queue
} // casual

#endif // QUEUE_H_
