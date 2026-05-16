//!
//! Copyright (c) 2026, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/header.h"

#include "common/buffer/type.h"

namespace casual
{
   //! some general types to help with compatibility between different versions of messages.
   namespace common::message::compatibility
   {
      //! Represents a payload, without the header. This is used for compatibility between 
      //! different versions of messages, where the header might be moved around or changed.
      struct Payload
      {
         Payload() = default;

         //! conversion ctor from common::buffer::Payload, to be used in message transformations.
         inline Payload( common::buffer::Payload&& payload) 
            : type( std::move( payload.type)), data( std::move( payload.data)) 
         {}

         //! conversion operator to common::buffer::Payload, to be used in message transformations.
         inline operator common::buffer::Payload() &&
         {
            return common::buffer::Payload{ 
               .type = std::move( type), 
               .data = std::move( data)};
         }

         std::string type;
         platform::binary::type data;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( type);
            CASUAL_SERIALIZE( data);
         )
      };

   } // common::message::compatibility
   
} // casual
