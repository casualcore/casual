//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "casual/platform.h"
#include "common/message/type.h"
#include "common/memory.h"
#include "common/log/category.h"
#include "common/network/byteorder.h"
#include "common/exception/system.h"
#include "common/string.h"


#include <cstdint>
#include <array>

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace message
         {
            using size_type = platform::size::type;

            namespace complete
            {
               namespace network
               {
                  struct Header
                  {
                     using host_type_type = platform::ipc::message::type;
                     using network_type_type = common::network::byteorder::type<host_type_type>;

                     using host_uuid_type = Uuid::uuid_type;
                     using network_uuid_type = host_uuid_type;

                     using host_size_type = platform::size::type;
                     using network_size_type = common::network::byteorder::type<host_size_type>;

                     network_type_type type = 0;
                     network_uuid_type correlation;
                     network_size_type size = 0;

                     static_assert( sizeof( network_type_type) ==  8, "Wrong size for type");
                     static_assert( sizeof( network_uuid_type) == 16, "Wrong size for uuid");
                     static_assert( sizeof( network_size_type) ==  8, "Wrong size for size");


                     CASUAL_CONST_CORRECT_SERIALIZE(
                     {
                        CASUAL_SERIALIZE( type);
                        CASUAL_SERIALIZE( correlation);
                        CASUAL_SERIALIZE( size);
                     })
                  };

                  static_assert( std::is_trivially_copyable< Header>::value, "Complete::Header needs to be trivially copyable" );

                  namespace header
                  {
                     constexpr auto size() { return sizeof( Header);}
                     static_assert( size() == 32, "Wrong size for header");
                  } // header

               } // network
            } // complete

            struct Complete
            {
               using message_type_type = common::message::Type;
               using payload_type = platform::binary::type;
               using range_type = decltype( range::make( payload_type::iterator(), 0));

               Complete();
               Complete( common::message::Type type, const Uuid& correlation);
               Complete( const complete::network::Header& header);

               template< typename Chunk>
               Complete( common::message::Type type, const Uuid& correlation, size_type size, Chunk&& chunk) :
                  type{ type}, correlation{ correlation},
                  payload( size), m_unhandled{ range::make( payload)}
               {
                  add( std::forward< Chunk>( chunk));
               }

               complete::network::Header header() const;

               Complete( Complete&&) noexcept;
               Complete& operator = ( Complete&&) noexcept;

               Complete( const Complete&) = delete;
               Complete& operator = ( const Complete&) = delete;


               explicit operator bool() const;

               bool complete() const;

               inline size_type size() const { return payload.size();}

               message_type_type type = message_type_type::absent_message;
               Uuid correlation;
               payload_type payload;

               //! @param transport
               template< typename Chunk>
               void add( Chunk&& chunk)
               {
                  const auto size = std::distance( std::begin( chunk), std::end( chunk));

                  // Some sanity checks
                  if( Complete::size() < offset( chunk) + size)
                  {
                     throw exception::system::invalid::Argument{
                        string::compose( "added chunk is out of bounds - size: ", payload.size())
                     };
                  }

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
               {
                  CASUAL_SERIALIZE( type);
                  CASUAL_SERIALIZE( correlation);
                  CASUAL_SERIALIZE( payload);
               })

               friend std::ostream& operator << ( std::ostream& out, const Complete& value);
               friend void swap( Complete& lhs, Complete& rhs);

               friend bool operator == ( const Complete& complete, const Uuid& correlation);
               friend inline bool operator == ( const Uuid& correlation, const Complete& complete)
               {
                  return complete == correlation;
               }

            private:
               std::vector< range_type> m_unhandled;
            };

            static_assert( traits::is_movable< Complete>::value, "not movable");

         } // message
      } // communication
   } // common
} // casual


