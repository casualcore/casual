//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"
#include "casual/platform.h"

namespace casual
{
   namespace common::message::counter
   {
      namespace add
      {
         void sent( message::Type type) noexcept;
         void received( message::Type type) noexcept;
         
      } // add
         
      struct Entry
      {
         Entry() = default;
         inline Entry( message::Type type) : type{ type} {}

         message::Type type{};
         platform::size::type sent{};
         platform::size::type received{};

         inline friend bool operator == ( const Entry& lhs, message::Type rhs) { return lhs.type == rhs;}

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( type);
            CASUAL_SERIALIZE( sent);
            CASUAL_SERIALIZE( received);
         )
      };

      //
      std::vector< Entry> entries() noexcept;

      
      using Request = basic_request< Type::counter_request>;
      
      using base_reply = basic_reply< Type::counter_reply>;
      struct Reply : base_reply
      {
         using base_reply::base_reply;

         std::vector< Entry> entries;

         CASUAL_CONST_CORRECT_SERIALIZE(
            base_reply::serialize( archive);
            CASUAL_SERIALIZE( entries);
         )
      };

               
   } // common::message::counter
   
   namespace common::message::reverse
   {
      template<>
      struct type_traits< counter::Request> : detail::type< counter::Reply> {};
      
   } // common::message::reverse

} // casual
