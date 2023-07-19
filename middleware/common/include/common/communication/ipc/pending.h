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
      namespace detail
      {
         namespace serialize
         {
            template< typename M>
            auto message( M&& message)
            {
               if constexpr( std::is_same_v< std::remove_cvref_t< M>, ipc::message::Complete>)
               {
                  static_assert( std::is_rvalue_reference_v< M>, "message::Complete needs to be rvalue reference");
                  return std::forward< M>( message);
               }
               else
                  return common::serialize::native::complete< ipc::message::Complete>( std::forward< M>( message));
            }
         } // serialize
         
      } // detail

      template< typename Destination>
      struct basic_message
      {
         template< typename M>
         inline basic_message( const Destination& destination, M&& message)
            : destination{ destination}, complete{ detail::serialize::message( std::forward< M>( message))}
         {}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( destination);
            CASUAL_SERIALIZE( complete);
         )

         template< typename D>
         friend bool operator == ( const basic_message& lhs, const D& rhs) { return lhs.destination == rhs;}

         Destination destination;
         message::Complete complete;
      };

      using Message = basic_message< process::Handle>;

      template< typename Destination>
      struct basic_holder
      {
         using message_type = basic_message< Destination>;

         basic_holder() = default;
         basic_holder( std::vector< message_type> messages) : m_messages{ std::move( messages)} {}

         template< typename M>
         inline auto& add( const Destination& destination, M&& message)
         {
            return m_messages.emplace_back( destination, std::forward< M>( message)).complete.correlation();
         }

         inline auto& add( message_type&& message)
         {
            m_messages.push_back( std::move( message));
            return m_messages.back().complete.correlation();
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

         std::vector< message_type> m_messages;
      };

      using Holder = basic_holder< process::Handle>;
      
   } // common::communication::ipc::pending
} // casual