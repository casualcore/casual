//!
//! message.h
//!
//! Created on: Nov 23, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_API_MESSAGE_H_
#define CASUAL_QUEUE_API_MESSAGE_H_

#include "sf/namevaluepair.h"
#include "sf/platform.h"



#include <string>

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
         sf::platform::time_point available = sf::platform::time_point::min();

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            archive & CASUAL_MAKE_NVP( properties);
            archive & CASUAL_MAKE_NVP( reply);
            archive & CASUAL_MAKE_NVP( available);
         })

      };

      struct Selector
      {
         //!
         //! If empty -> not used
         //! If not empty -> the first message that gets a match against the regexp is dequeued.
         //!
         std::string properties;

         //!
         //! If not 'null', the first message that has this particular id is dequeued
         //!
         sf::platform::Uuid id;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            archive & CASUAL_MAKE_NVP( properties);
            archive & CASUAL_MAKE_NVP( id);
         })
      };

      struct Payload
      {
         struct type_t
         {
            std::string type;
            std::string subtype;

            CASUAL_CONST_CORRECT_SERIALIZE(
            {
               archive & CASUAL_MAKE_NVP( type);
               archive & CASUAL_MAKE_NVP( subtype);
            })
         } type;

         sf::platform::binary_type data;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            archive & CASUAL_MAKE_NVP( type);
            archive & CASUAL_MAKE_NVP( data);
         })
      };


      template< typename P>
      struct basic_message
      {
         using payload_type = P;
         using id_type = common::Uuid;

         basic_message( id_type id, Attributes attributes, payload_type payload)
            : id( std::move( id)), attributes( std::move( attributes)), payload( std::move( payload)) {}

         basic_message( payload_type payload)
            : payload( std::move( payload)) {}

         basic_message() = default;

         id_type id;
         Attributes attributes;
         payload_type payload;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            archive & CASUAL_MAKE_NVP( id);
            archive & CASUAL_MAKE_NVP( attributes);
            archive & CASUAL_MAKE_NVP( payload);
         })
      };


      using Message = basic_message< Payload>;


      namespace peek
      {
         namespace message
         {
            struct Information
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

         } // message

         /*
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
         */
      } // peek

      namespace xatmi
      {
         struct Payload
         {
            Payload() = default;
            Payload( common::platform::raw_buffer_type buffer, common::platform::raw_buffer_size size)
              : buffer( buffer), size( size) {}

            Payload( common::platform::raw_buffer_type buffer)
              : buffer( buffer), size( 0) {}

            common::platform::raw_buffer_type buffer;
            common::platform::raw_buffer_size size;
         };

         using Message = basic_message< Payload>;

      } // xatmi

   } // queue

} // casual

#endif // MESSAGE_H_
