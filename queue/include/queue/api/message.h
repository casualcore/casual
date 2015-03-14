//!
//! message.h
//!
//! Created on: Nov 23, 2014
//!     Author: Lazan
//!

#ifndef CASUAL_QUEUE_API_MESSAGE_H_
#define CASUAL_QUEUE_API_MESSAGE_H_

#include "sf/namevaluepair.h"

#include "common/platform.h"
#include "common/uuid.h"


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
         common::platform::time_point available = common::platform::time_point::min();

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            archive & CASUAL_MAKE_NVP( properties);
            archive & CASUAL_MAKE_NVP( reply);
            archive & CASUAL_MAKE_NVP( available);
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

         common::platform::binary_type data;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            archive & CASUAL_MAKE_NVP( type);
            archive & CASUAL_MAKE_NVP( data);
         })
      };

      struct Message
      {
         common::Uuid id;
         Attributes attribues;
         Payload payload;

         CASUAL_CONST_CORRECT_SERIALIZE(
         {
            archive & CASUAL_MAKE_NVP( id);
            archive & CASUAL_MAKE_NVP( attribues);
            archive & CASUAL_MAKE_NVP( payload);
         })
      };

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
      } // peek

   } // queue

} // casual

#endif // MESSAGE_H_
