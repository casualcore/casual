//!
//! casual
//!

#ifndef CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_H_
#define CASUAL_MIDDLEWARE_COMMON_INCLUDE_COMMON_COMMUNICATION_H_


#include "common/platform.h"
#include "common/message/type.h"
#include "common/memory.h"
#include "common/log/category.h"


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

            template< typename P>
            struct basic_transport
            {
               using policy_type = P;
               using correalation_type = Uuid::uuid_type;

               struct header_t
               {


                  //!
                  //! The message correlation id
                  //!
                  correalation_type correlation;

                  //!
                  //! which offset this transport message represent of the complete message
                  //!
                  std::int64_t offset;

                  //!
                  //! Size of payload in this transport message
                  //!
                  std::int64_t count;

                  //!
                  //! size of the logical complete message
                  //!
                  std::int64_t complete_size;
               };



               static constexpr std::int64_t max_message_size() { return policy_type::message_size();}
               static constexpr std::int64_t header_size()
               {
                  //
                  // We let the policy decide the size of the header, since on ipc the message type is not
                  // included in the size of what is being sent and receive, while it is on tcp.
                  //
                  return policy_type::header_size( sizeof( header_t), sizeof( platform::ipc::message::type));
               }
               static constexpr std::int64_t max_payload_size() { return max_message_size() - header_size();}


               using payload_type = std::array< char, max_payload_size()>;
               using range_type = range::type_t< payload_type>;
               using const_range_type = range::const_type_t< payload_type>;

               struct message_t
               {
                  //!
                  //! Which logical type of message this transport is carrying
                  //!
                  //! @attention has to be the first bytes in the message
                  //!
                  platform::ipc::message::type type;

                  header_t header;
                  payload_type payload;

               } message;


               static_assert( max_message_size() - max_payload_size() < max_payload_size(), "Payload is to small");
               static_assert( std::is_pod< message_t>::value, "Message has be a POD");
               //static_assert( sizeof( message_t) - header_size() == max_payload_size(), "something is wrong with padding");


               basic_transport() { memory::set( message);}

               basic_transport( common::message::Type type, std::size_t complete_size) : basic_transport()
               {
                  basic_transport::type( type);
                  message.header.complete_size = complete_size;
               }

               basic_transport( common::message::Type type) : basic_transport() { basic_transport::type( type);}



               //!
               //! @return the message type
               //!
               inline common::message::Type type() const { return static_cast< common::message::Type>( message.type);}

               //!
               //! Sets the message type
               //!
               //! @param type type to set
               //!
               void type( common::message::Type type) { message.type = static_cast< platform::ipc::message::type>( type);}


               const_range_type payload() const
               {
                  return range::make( std::begin( message.payload), message.header.count);
               }


               inline const correalation_type& correlation() const { return message.header.correlation;}
               inline correalation_type& correlation() { return message.header.correlation;}

               //!
               //! @return payload size
               //!
               inline std::size_t pyaload_size() const { return message.header.count;}

               //!
               //! @return the offset of the logical complete message this transport
               //!    message represent.
               //!
               inline std::size_t pyaload_offset() const { return message.header.offset;}


               //!
               //! @return the size of the complete logical message
               //!
               inline std::size_t complete_size() const { return message.header.complete_size;}
               inline void complete_size( std::size_t size) const { message.header.complete_size = size;}


               //!
               //! @return the total size of the transport message including header.
               //!
               inline std::size_t size() const { return header_size() + pyaload_size();}


               //!
               //! Indication if this transport message is the last of the logical message.
               //!
               //! @return true if this transport message is the last of the logical message.
               //!
               //! @attention this does not give any guarantees that no more transport messages will arrive...
               //!
               bool last() const { return message.header.offset + message.header.count == message.header.complete_size;}


               template< typename Iter>
               void assign( Iter first, Iter last)
               {
                  message.header.count = std::distance( first, last);
                  assert( first <= last && message.header.count <=  max_payload_size());

                  std::copy( first, last, std::begin( message.payload));
               }

               template< typename Pol>
               friend std::ostream& operator << ( std::ostream& out, const basic_transport< Pol>& value);
            };



            template< typename P>
            std::ostream& operator << ( std::ostream& out, const basic_transport< P>& value)
            {
               return out << "{ type: " << value.type()
                     << ", correlation: " << uuid::string( value.correlation())
                     << ", offset: " << value.pyaload_offset()
                     << ", payload.size: " << value.pyaload_size()
                     << ", complete_size: " << value.complete_size()
                     << ", header-size: " << value.header_size()
                     << ", transport-size: " <<  value.size()
                     << ", max-size: " << value.max_message_size() << "}";
            }


            struct Complete
            {
               using message_type_type = common::message::Type;
               using payload_type = platform::binary::type;
               using range_type = decltype( range::make( payload_type::iterator(), 0));

               Complete();
               Complete( message_type_type type, const Uuid& correlation);

               template< typename Policy>
               Complete( const basic_transport< Policy>& transport) :
                  type{ transport.type()}, correlation{ transport.correlation()},
                  payload( transport.complete_size()), m_unhandled{ range::make( payload)}
               {
                  add( transport);
                  //add( std::forward< T>( transport));
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




               //! @param transport
               template< typename Policy>
               void add( const basic_transport< Policy>& transport)
               {
                  if( payload.size() != transport.complete_size())
                  {
                     log::category::error << "payload.size(): " << payload.size() << " - transport.complete_size(): " << transport.complete_size() << '\n';
                  }
                  assert( payload.size() == transport.complete_size());

                  auto source = transport.payload();
                  auto destination = range::make( std::begin( payload) + transport.pyaload_offset(), source.size());

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
                        // transport message came out of order
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



               const std::vector< range_type>& unhandled() { return m_unhandled;}

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
