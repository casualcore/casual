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
#include "common/uuid.h"
#include "common/algorithm.h"


namespace casual
{
   namespace common::communication::ipc::message
   {
      struct Complete
      {
         using message_type_type = common::message::Type;
         using payload_type = platform::binary::type;
         using range_type = range::type_t< payload_type>;

         Complete() = default;
         Complete( common::message::Type type, const Uuid& correlation);
         Complete( common::message::Type type, const Uuid& correlation, payload_type&& payload);

         template< typename Chunk>
         Complete( common::message::Type type, const Uuid& correlation, platform::size::type size, Chunk&& chunk)
            : payload( size), m_type{ type}, m_correlation{ correlation},
               m_unhandled{ range::make( payload)}
         {
            add( std::forward< Chunk>( chunk));
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

         payload_type payload;

         //! @param transport
         template< typename Chunk>
         void add( Chunk&& chunk)
         {
            const auto size = std::distance( std::begin( chunk), std::end( chunk));

            // Some sanity checks
            if( Complete::size() < offset( chunk) + size)
               code::raise::error( code::casual::internal_out_of_bounds, "added chunk is out of bounds - size: ", payload.size());

            auto destination = range::make( std::begin( payload) + offset( chunk), size);

            algorithm::copy( chunk, std::begin( destination));


            {
               range_type splitted;

               for( auto& range : m_unhandled)
               {
                  auto result = range::position::subtract( range, destination);
                  range = std::get< 0>( result);
                  splitted =  std::get< 1>( result);
               }

               if( splitted)
               {
                  // chunk is out of order
                  m_unhandled.push_back( splitted);
               }
            }

            // Remove those who have been handled.
            //
            // This should work, doesn't on g++ 5.4
            // auto last = std::partition( std::begin( m_unhandled), std::end( m_unhandled), []( const auto& r){ return ! r.empty();});
            using unhandled_type = decltype( *std::begin( m_unhandled));
            auto last = std::partition( std::begin( m_unhandled), std::end( m_unhandled), []( unhandled_type& r){ return ! r.empty();});

            m_unhandled.erase( last, std::end( m_unhandled));
         }

         inline const std::vector< range_type>& unhandled() { return m_unhandled;}

         //! So we can send complete messages as part of other
         //! messages
         CASUAL_CONST_CORRECT_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_type, "type");
            CASUAL_SERIALIZE_NAME( m_correlation, "correlation");
            CASUAL_SERIALIZE( payload);
         )

         friend std::ostream& operator << ( std::ostream& out, const Complete& value);

         friend bool operator == ( const Complete& complete, const Uuid& correlation);
         friend inline bool operator == ( const Uuid& correlation, const Complete& complete)
         {
            return complete == correlation;
         }

         friend inline bool operator == ( const Complete& lhs, common::message::Type rhs) { return lhs.m_type == rhs;}
         friend inline bool operator == ( common::message::Type lhs, const Complete& rhs) { return lhs == rhs.m_type;}

      private:

         common::message::Type m_type{};
         Uuid m_correlation;
         std::vector< range_type> m_unhandled;
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