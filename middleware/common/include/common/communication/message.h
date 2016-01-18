//!
//! communication.h
//!
//! Created on: Jan 3, 2016
//!     Author: Lazan
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_H_


#include "common/platform.h"
#include "common/message/type.h"
#include "common/memory.h"


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
            // common::platform::message_size

            template< std::size_t message_size>
            struct basic_transport
            {
               typedef platform::message_type_type message_type_type;
               typedef Uuid::uuid_type correalation_type;



               struct header_t
               {
                  correalation_type correlation;

                  //!
                  //! which offset this transport message represent of the complete message
                  //!
                  std::uint64_t offset;

                  //!
                  //! Size of payload in this transport message
                  //!
                  std::uint64_t count;

                  //!
                  //! size of the logical complete message
                  //!
                  std::uint64_t complete_size;
               };

               enum
               {
                  message_max_size = message_size,
                  header_size = sizeof( header_t),
                  payload_max_size = message_max_size - header_size,

               };

               using payload_type = std::array< char, payload_max_size>;
               using range_type = range::type_t< payload_type>;

               struct message_t
               {


                  //
                  // type has to be first!
                  //
                  message_type_type type;
                  header_t header;
                  payload_type payload;

               } message;


               static_assert( message_max_size - payload_max_size < payload_max_size, "Payload is to small");
               static_assert( std::is_pod< message_t>::value, "Message has be a POD");
               static_assert( sizeof( message_t) - sizeof( message_type_type) == message_max_size, "something is wrong with padding");


               basic_transport() { memory::set( message);}
               basic_transport( common::message::Type type) : basic_transport() { basic_transport::type( type);}


               //!
               //! @return the message type
               //!
               common::message::Type type() const { return static_cast< common::message::Type>( message.type);}

               //!
               //! Sets the message type
               //!
               //! @param type type to set
               //!
               void type( common::message::Type type) { message.type = static_cast< decltype( message.type)>( type);}


               range_type payload() const
               {
                  return range::make( std::begin( message.payload), message.header.count);
               }


               //!
               //! @return payload size
               //!
               std::size_t size() const { return message.header.count;}


               //!
               //! Indication if this transport message is the last of the logical message.
               //!
               //! @return true if this transport message is the last of the logical message.
               //!
               //! @attention this gives not any guarantees that no more transport messages will arrive...
               //!
               bool last() const { return message.header.offset + message.header.count == message.header.complete_size;}


               template< typename Iter>
               void assign( Iter first, Iter last)
               {
                  message.header.count = std::distance( first, last);
                  assert( first <= last && message.header.count <= payload_max_size);

                  std::copy( first, last, std::begin( message.payload));
               }

               //template< std::size_t message_size>
               //friend std::ostream& operator << ( std::ostream& out, const basic_transport< message_size>& value);
               template< std::size_t m_size>
               friend std::ostream& operator << ( std::ostream& out, const basic_transport< m_size>& value);

               /*
               friend bool send( id_type id, const Transport& transport, long flags);
               friend bool receive( id_type id, Transport& transport, long flags);
               friend bool ignore::signal::send( id_type id, const Transport& transport, long flags);
               friend bool ignore::signal::receive( id_type id, Transport& transport, long flags);
               */


            private:

               //std::size_t m_size = 0;
            };


            template< std::size_t message_size>
            std::ostream& operator << ( std::ostream& out, const basic_transport< message_size>& value)
            {
               using transport_type = basic_transport< message_size>;

               return out << "{type: " << value.message.type << ", correlation: " << common::uuid::string( value.message.header.correlation)
                     << ", offset: " << value.message.header.offset
                     << ", count: " << value.message.header.count
                     << ", complete_size: " << value.message.header.complete_size
                     << ", header-size: " << transport_type::header_size
                     << ", max-size: " << transport_type::message_max_size << "}";
            }


            struct Complete
            {
               using message_type_type = common::message::Type;
               using payload_type = platform::binary_type;
               using range_type = decltype( range::make( payload_type::iterator(), 0));

               Complete() = default;

               Complete( message_type_type type, const Uuid& correlation) : type{ type}, correlation{ correlation} {}

               template< typename T>
               Complete( T&& transport) :
                  type{ common::message::Type( transport.message.type)}, correlation{ transport.message.header.correlation},
                  payload( transport.message.header.complete_size), m_unhandled{ range::make( payload)}
               {
                  add( transport);
               }


               Complete( Complete&&) noexcept;
               Complete& operator = ( Complete&&) noexcept;

               Complete( const Complete&) = delete;
               Complete& operator = ( const Complete&) = delete;


               explicit operator bool() const;

               bool complete() const;

               message_type_type type = message_type_type::absent_message;
               Uuid correlation;
               payload_type payload;

               friend std::ostream& operator << ( std::ostream& out, const Complete& value);


               //! @param transport
               template< typename T>
               void add( T& transport)
               {
                  assert( payload.size() == transport.message.header.complete_size);

                  auto source = transport.payload();
                  auto destination = range::make( std::begin( payload) + transport.message.header.offset, source.size());

                  range::copy( source, std::begin( destination));


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
                        // transport message came out of orderl
                        //
                        m_unhandled.push_back( splitted);
                     }
                  }

                  //
                  // Remove those who have been handled.
                  //
                  auto last = std::partition( std::begin( m_unhandled), std::end( m_unhandled), []( const range_type r){ return ! r.empty();});
                  m_unhandled.erase( last, std::end( m_unhandled));
               }

               friend void swap( Complete& lhs, Complete& rhs);

               const std::vector< range_type>& unhandled() { return m_unhandled;}

            private:


               std::vector< range_type> m_unhandled;

            };

         } // message

      } // communication

   } // common

} // casual

#endif // CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_H_
