//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "casual/platform.h"

#include "common/serialize/native/binary.h"
#include "common/serialize/native/complete.h"

#include "common/message/type.h"
#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/algorithm/container.h"


namespace casual
{
   namespace common::communication::ipc::message
   {
      namespace transport
      {
         struct Header
         {
            //! Which logical type of message this transport is carrying
            //! @attention has to be the first bytes in the message
            common::message::Type type;

            using correlation_type = Uuid::uuid_type;

            //! The message correlation id
            correlation_type correlation;

            //! which offset this transport message represent of the complete message
            std::int64_t offset;

            //! Size of payload in this transport message
            std::int64_t count;

            //! size of the logical complete message
            std::int64_t size;

            friend std::ostream& operator << ( std::ostream& out, const Header& value);
         };

         constexpr std::int64_t max_message_size() { return platform::ipc::transport::size;}
         constexpr std::int64_t header_size() { return sizeof( Header);}
         constexpr std::int64_t max_payload_size() { return max_message_size() - header_size();}

      } // transport

      struct Transport
      {
         using correlation_type = Uuid::uuid_type;

         using payload_type = std::array< char, transport::max_payload_size()>;
         using range_type = range::type_t< payload_type>;
         using const_range_type = range::const_type_t< payload_type>;

         struct message_t
         {
            transport::Header header;
            payload_type payload;

         } message{}; // note the {} which initialize the memory to 0:s


         static_assert( transport::max_message_size() - transport::max_payload_size() < transport::max_payload_size(), "Payload is to small");
         static_assert( std::is_trivially_copyable< message_t>::value, "Message has to be trivially copyable");
         static_assert( ( transport::header_size() + transport::max_payload_size()) == transport::max_message_size(), "something is wrong with padding");


         inline Transport() = default;

         inline Transport( common::message::Type type, platform::size::type size, const strong::correlation::id& correlation)
         {
            message.header.type = type;
            message.header.size = size;
            correlation.value().copy( message.header.correlation);
         }

         //! @return the message type
         inline common::message::Type type() const { return static_cast< common::message::Type>( message.header.type);}

         inline const_range_type payload() const
         {
            return range::make( std::begin( message.payload), message.header.count);
         }

         inline const auto& correlation() const { return message.header.correlation;}
         inline auto& correlation() { return message.header.correlation;}

         //! @return payload size
         inline platform::size::type payload_size() const { return message.header.count;}

         //! @return the offset of the logical complete message this transport
         //!    message represent.
         inline platform::size::type payload_offset() const { return message.header.offset;}
         //inline auto offset() const noexcept { return message.header.offset;}

         //! @return the size of the complete logical message
         inline platform::size::type complete_size() const noexcept{ return message.header.size;}

         //! @return the total size of the transport message including header.
         inline platform::size::type size() const noexcept { return transport::header_size() + payload_size();}

         inline void* data() noexcept { return static_cast< void*>( &message);}
         inline const void* data() const noexcept { return static_cast< const void*>( &message);}

         inline void* header_data() noexcept { return static_cast< void*>( &message.header);}
         inline void* payload_data() noexcept { return static_cast< void*>( &message.payload);}


         //! Indication if this transport message is the last of the logical message.
         //!
         //! @return true if this transport message is the last of the logical message.
         //! @attention this does not give any guarantees that no more transport messages will arrive...
         inline bool last() const noexcept { return message.header.offset + message.header.count == message.header.size;}


         auto begin() noexcept { return std::begin( message.payload);}
         inline auto begin() const noexcept { return std::begin( message.payload);}
         auto end() noexcept { return begin() + message.header.count;}
         auto end() const noexcept { return begin() + message.header.count;}

         template< typename R>
         void assign( R&& range)
         {
            message.header.count = range.size();
            assert( message.header.count <=  transport::max_payload_size());

            algorithm::copy( range, std::begin( message.payload));
         }

         template< typename R>
         void assign( R&& range, platform::size::type offset)
         {
            message.header.count = range.size();
            assert( message.header.count <=  transport::max_payload_size());
            message.header.offset = offset;

            algorithm::copy( range, std::begin( message.payload));
         }

         friend std::ostream& operator << ( std::ostream& out, const Transport& value);
      };

         

      struct Complete
      {
         using range_type = range::type_t< platform::binary::type>;

         Complete() = default;
         Complete( common::message::Type type, const strong::correlation::id& correlation);
         Complete( common::message::Type type, const strong::correlation::id& correlation, platform::binary::type&& payload);

         template< typename Chunk>
         Complete( common::message::Type type, const strong::correlation::id& correlation, platform::size::type size, Chunk&& chunk)
            : payload( size), m_type{ type}, m_correlation{ correlation}
         {
            add( std::forward< Chunk>( chunk));
         }

         Complete( const Transport& transport)
            : payload( transport.complete_size()), m_type{ transport.type()}, m_correlation{ transport.correlation()}
         {
            add( transport);
         }


         Complete( Complete&&) noexcept = default;
         Complete& operator = ( Complete&&) noexcept = default;

         Complete( const Complete&) = delete;
         Complete& operator = ( const Complete&) = delete;

         explicit operator bool() const;

         bool complete() const noexcept;

         inline platform::size::type size() const noexcept { return payload.size();}

         inline auto type() const noexcept { return m_type;}
         inline auto& correlation() const noexcept { return m_correlation;}
         inline auto offset() noexcept { return m_offset;}

         platform::binary::type payload;

         //! Adds a transport message.
         //!   ignores the transport if offset is smaller than current
         void add( const Transport& transport);

         friend inline bool operator == ( const Complete& lhs, const strong::correlation::id& rhs) { return lhs.m_correlation == rhs;};
         friend inline bool operator == ( const strong::correlation::id& lhs, const Complete& rhs) { return rhs == lhs;}

         friend inline bool operator == ( const Complete& lhs, common::message::Type rhs) { return lhs.m_type == rhs;}
         friend inline bool operator == ( common::message::Type lhs, const Complete& rhs) { return lhs == rhs.m_type;}

         //! So we can send complete messages as part of other
         //! messages
         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_type, "type");
            CASUAL_SERIALIZE_NAME( m_correlation, "correlation");
            CASUAL_SERIALIZE( payload);
         )

         friend std::ostream& operator << ( std::ostream& out, const Complete& value);

      private:

         common::message::Type m_type{};
         strong::correlation::id m_correlation;
         platform::size::type m_offset{};
      };

      static_assert( traits::is_movable< Complete>::value, "not movable");

      template< typename M>
      Complete& operator >> ( Complete& complete, M& message)
      {
         assert( complete.type() == message.type());

         message.correlation = complete.correlation();

         auto archive = serialize::native::binary::reader( complete.payload);
         archive >> message;

         return complete;
      }

      namespace complete
      {
         namespace send
         {
            struct Chunk
            {
               Complete::range_type range;
               platform::size::type offset{};

               explicit operator bool() const noexcept { return predicate::boolean( range);}
            };
         } // send

         struct Send
         {
            Send( Complete&& complete) : m_complete{ std::move( complete)} {};
            
            //! @returns an empty transport message
            inline auto transport() const noexcept
            {
               return message::Transport{ m_complete.type(), m_complete.size(), m_complete.correlation()};
            }

            //! access the first chunk
            //! @return chunk of transport::max_payload_size() size, smaller chunk if 
            //!    there is less left, including empty chunk (if all has been consumed)
            inline auto front() noexcept
            {
               return send::Chunk{ range::make( std::begin( m_complete.payload) + m_offset, left()), m_offset};
            }

            //! removes ths first chunk
            void pop() noexcept
            {
               m_offset += left();
            }

            inline const auto& correlation() const noexcept { return m_complete.correlation();}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_complete, "complete");
               CASUAL_SERIALIZE_NAME( m_offset, "offset");
            )

         private:
            platform::size::type left() const noexcept
            {
               const auto left = m_complete.size() - m_offset;
               return left < transport::max_payload_size() ? left : transport::max_payload_size();
            }

            Complete m_complete;
            platform::size::type m_offset{};
         };
/*
         struct Receive
         {
            Receive( common::message::Type type, const strong::correlation::id& correlation)
               : m_complete{ type, correlation} {};

         private:
            Complete m_complete;
            platform::size::type m_offset{};
         };
         */
      } // complete

   } // common::communication::ipc::message

   namespace common::serialize::native::customization
   {     
      template<>
      struct point< communication::ipc::message::Complete>
      {
         using writer = binary::create::Writer;
         using reader = binary::create::Reader;
      };

   } //common::serialize::native::customization
           

} // casual