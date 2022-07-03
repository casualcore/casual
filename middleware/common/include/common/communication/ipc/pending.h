//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc/message.h"

namespace casual
{
   namespace common::communication::ipc::pending
   {
      template< typename Destination = process::Handle>
      struct Holder
      {
         template< typename M>
         inline auto& add( const Destination& destination, M&& message)
         {
            if constexpr( std::is_same_v< traits::remove_cvref_t< M>, ipc::message::Complete>)
            {
               static_assert( std::is_rvalue_reference_v< M>, "pending::Holder::send meeds message::Complete to be rvalue reference");
               return m_messages.emplace_back( destination, std::move( message)).complete.correlation();
            }
            else
            {
               return m_messages.emplace_back( destination, serialize::native::complete< ipc::message::Complete>( std::forward< M>( message))).complete.correlation();
            }
         }

         template< typename Multiplex>
         void send( Multiplex& multiplex)
         {
            for( auto&& message : std::exchange( m_messages, {}))
               multiplex.send( message.destination, std::move( message.complete));
         }

         template< typename D>
         message::Complete next( const D& destination) noexcept
         {
            if( auto found = algorithm::find( m_messages, destination))
               return algorithm::container::extract( m_messages, std::begin( found)).complete;

            return {};
         }

         template< typename D>
         auto remove( const D& destination)
         {
            return algorithm::container::extract( 
               m_messages, 
               algorithm::filter( m_messages, [ &destination]( auto& message){ return message.destination == destination;}));
         }

         inline auto empty() const noexcept { return m_messages.empty();}
         inline platform::size::type size() const noexcept { return m_messages.size();}

         auto& messages() const noexcept { return m_messages;}
         auto begin() const noexcept { return std::begin( m_messages);}
         auto end() const noexcept { return std::end( m_messages);}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_messages, "messages");
         )

      private:

         struct Message
         {
            inline Message( const Destination& destination, message::Complete&& complete)
               : destination{ destination}, complete{ std::move( complete)}  
            {}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( destination);
               CASUAL_SERIALIZE( complete);
            )

            template< typename D>
            friend bool operator == ( const Message& lhs, const D& rhs) { return lhs.destination == rhs;}

            Destination destination;
            message::Complete complete;
         };

         std::vector< Message> m_messages;
      };
      
   } // common::communication::ipc::pending
} // casual