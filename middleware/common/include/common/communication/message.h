//!
//! casual
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_H_


#include "common/platform.h"
#include "common/message/type.h"
#include "common/memory.h"
#include "common/log/category.h"
#include "common/network/byteorder.h"


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
            namespace complete
            {
               namespace network
               {
                  struct Header
                  {
                     platform::ipc::message::type type = 0;
                     Uuid::uuid_type correlation;
                     platform::binary::size::type size = 0;

                     friend std::ostream& operator << ( std::ostream& out, const Header& value);

                  };
                  static_assert( std::is_trivially_copyable< Header>::value, "Complete::Header needs to be trivially copyable" );

                  namespace header
                  {
                     constexpr auto size() { return sizeof( Header);}
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
               Complete( common::message::Type type, const Uuid& correlation, std::size_t size, Chunk&& chunk) :
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

               message_type_type type = message_type_type::absent_message;
               Uuid correlation;
               payload_type payload;




               //! @param transport
               template< typename Chunk>
               void add( Chunk&& chunk)
               {

                  const auto size = std::distance( std::begin( chunk), std::end( chunk));

                  //
                  // Some sanity checks
                  //
                  if( payload.size() < offset( chunk) + size)
                  {
                     throw exception::invalid::Argument{
                        "communication::message::Complete: added chunk is out of bounds", CASUAL_NIP( payload.size())
                     };
                  }


                  auto destination = range::make( std::begin( payload) + offset( chunk), size);

                  range::copy( chunk, std::begin( destination));


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
                        //
                        // chunk is out of order
                        //
                        m_unhandled.push_back( splitted);
                     }
                  }

                  //
                  // Remove those who have been handled.
                  //
                  auto last = std::partition( std::begin( m_unhandled), std::end( m_unhandled), []( const auto& r){ return ! r.empty();});
                  m_unhandled.erase( last, std::end( m_unhandled));
               }



               inline const std::vector< range_type>& unhandled() { return m_unhandled;}

               //!
               //! So we can send complete messages as part of other
               //! messages
               //!
               CASUAL_CONST_CORRECT_MARSHAL(
               {
                  archive & type;
                  archive & correlation;
                  archive & payload;
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

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_H_
