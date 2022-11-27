//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/discovery/state.h"

namespace casual
{
   //! @attention This is not to be regarded "stable", the model
   //!    could change at any version. 
   namespace domain::discovery::admin::model
   {
   inline namespace v1 {

      namespace pending
      {
         struct Message
         {
            common::message::Type type{};
            platform::size::type count{};

            friend bool operator == ( const Message& lhs, common::message::Type rhs) noexcept { return lhs.type == rhs;}

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( type);
               CASUAL_SERIALIZE( count);
            )
         };

         struct Destination
         {
            common::strong::ipc::id ipc;
            common::strong::socket::id socket;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( ipc);
               CASUAL_SERIALIZE( socket);
            )
         };

         struct Send
         {
            Destination destination;
            std::vector< Message> messages;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( destination);
               CASUAL_SERIALIZE( messages);
            )
         };
      } // pending

      struct Pending
      {
         std::vector< pending::Send> send;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( send);
         )
      };

      namespace metric
      {
         namespace message
         {
            struct Count
            {
               common::message::Type type{};
               platform::size::type value{};

               inline friend bool operator == ( const Count& lhs, common::message::Type rhs) noexcept { return lhs.type == rhs;}

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( type);
                  CASUAL_SERIALIZE( value);
               )
            };
            
         } // message
         
      } // metric

      struct Metric
      {
         struct
         {
            struct
            {
               std::vector< metric::message::Count> receive;
               std::vector< metric::message::Count> send;

               CASUAL_CONST_CORRECT_SERIALIZE(
                  CASUAL_SERIALIZE( receive);
                  CASUAL_SERIALIZE( send);
               )
            } count;

            CASUAL_CONST_CORRECT_SERIALIZE(
               CASUAL_SERIALIZE( count);
            )

         } message;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( message);
         )

      };

      struct Provider
      {
         state::provider::Abilities abilities{};
         common::process::Handle process;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( abilities);
            CASUAL_SERIALIZE( process);
         )
      };
      

      struct State
      {
         state::Runlevel runlevel;
         std::vector< Provider> providers;
         Pending pending;
         Metric metric;

         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( providers);
            CASUAL_SERIALIZE( metric);
         )

      };

   }
   } // domain::discovery::admin::model
} // casual