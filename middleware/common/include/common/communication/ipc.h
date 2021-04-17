//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc/message.h"
#include "common/communication/device.h"
#include "common/communication/socket.h"
#include "common/communication/log.h"

#include "casual/platform.h"
#include "common/strong/id.h"
#include "common/uuid.h"
#include "common/algorithm.h"
#include "common/memory.h"
#include "common/flag.h"


#include <sys/un.h>

namespace casual
{
   namespace common::communication::ipc
   {
      namespace message
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

            inline Transport( common::message::Type type, platform::size::type size)
            {
               message.header.type = type;
               message.header.size = size;
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

            //! @return the size of the complete logical message
            inline platform::size::type complete_size() const { return message.header.size;}

            //! @return the total size of the transport message including header.
            inline platform::size::type size() const { return transport::header_size() + payload_size();}

            inline void* data() { return static_cast< void*>( &message);}
            inline const void* data() const { return static_cast< const void*>( &message);}

            inline void* header_data() { return static_cast< void*>( &message.header);}
            inline void* payload_data() { return static_cast< void*>( &message.payload);}


            //! Indication if this transport message is the last of the logical message.
            //!
            //! @return true if this transport message is the last of the logical message.
            //! @attention this does not give any guarantees that no more transport messages will arrive...
            inline bool last() const { return message.header.offset + message.header.count == message.header.size;}

            auto begin() { return std::begin( message.payload);}
            inline auto begin() const { return std::begin( message.payload);}
            auto end() { return begin() + message.header.count;}
            auto end() const { return begin() + message.header.count;}

            template< typename R>
            void assign( R&& range)
            {
               message.header.count = range.size();
               assert( message.header.count <=  transport::max_payload_size());

               algorithm::copy( range, std::begin( message.payload));
            }

            friend std::ostream& operator << ( std::ostream& out, const Transport& value);
         };

         inline platform::size::type offset( const Transport& value) { return value.message.header.offset;}

      } // message


      struct Handle
      {

         explicit Handle( Socket&& socket, strong::ipc::id ipc);
         Handle( Handle&& other) noexcept;
         Handle& operator = ( Handle&& other) noexcept;
         ~Handle();

         inline const Socket& socket() const { return m_socket;}
         inline strong::ipc::id ipc() const { return m_ipc;}

         inline explicit operator bool () const { return ! m_socket.empty();}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_socket, "socket");
            CASUAL_SERIALIZE_NAME( m_ipc, "ipc");
         )

      private:
         Socket m_socket;
         strong::ipc::id m_ipc;
      };

      static_assert( sizeof( Handle) == sizeof( Socket) + sizeof( strong::ipc::id), "padding problem");

      struct Address
      {
         Address() = default;
         explicit Address( strong::ipc::id id);

         inline const ::sockaddr_un& native() const noexcept { return m_native;}

         inline const ::sockaddr* native_pointer() const noexcept { return reinterpret_cast< const ::sockaddr*>( &m_native);}
         inline auto native_size() const noexcept { return sizeof( m_native);}

         friend std::ostream& operator << ( std::ostream& out, const Address& rhs);

      private:
         ::sockaddr_un m_native = {};
      };

      namespace native
      {
         namespace detail
         {
            namespace create
            {
               namespace domain
               {
                  Socket socket();
               } // domain
            } // create
            namespace outbound
            {
               const Socket& socket();
            } // outbound
         } // detail

         enum class Flag : int
         {
            none = 0,
            non_blocking = platform::flag::value( platform::flag::tcp::no_wait)
         };

         bool send( const Socket& socket, const Address& destination, const message::Transport& transport, Flag flag);
         bool receive( const Handle& handle, message::Transport& transport, Flag flag);

         namespace blocking
         {
            bool send( const Socket& socket, const Address& destination, const message::Transport& transport);
            bool receive( const Handle& handle, message::Transport& transport);
         } // blocking

         namespace non
         {
            namespace blocking
            {
               bool send( const Socket& socket, const Address& destination, const message::Transport& transport);
               bool receive( const Handle& handle, message::Transport& transport);
            } // blocking
         } // non
      } // native


      namespace policy
      {
         using complete_type = message::Complete;
         using cache_type = std::vector< complete_type>;
         using cache_range_type = range::type_t< cache_type>;

         namespace blocking
         {
            cache_range_type receive( Handle& handle, cache_type& cache);
            strong::correlation::id send( const Socket& socket, const Address& destination, const complete_type& complete);
         } // blocking


         struct Blocking
         {

            template< typename Connector>
            cache_range_type receive( Connector&& connector, cache_type& cache)
            {
               return policy::blocking::receive( connector.handle(), cache);
            }

            template< typename Connector>
            auto send( Connector&& connector, const complete_type& complete)
            {
               return policy::blocking::send( connector.socket(), connector.destination(), complete);
            }
         };

         namespace non
         {
            namespace blocking
            {
               cache_range_type receive( Handle& handle, cache_type& cache);
               strong::correlation::id send( const Socket& socket, const Address& destination, const complete_type& complete);
            } // blocking

            struct Blocking
            {
               template< typename Connector>
               cache_range_type receive( Connector&& connector, cache_type& cache)
               {
                  return policy::non::blocking::receive( connector.handle(), cache);
               }

               template< typename Connector>
               auto send( Connector&& connector, const complete_type& complete)
               {
                  return policy::non::blocking::send( connector.socket(), connector.destination(), complete);
               }
            };

         } // non

      } // policy


      namespace inbound
      {
         struct Connector
         {
            using transport_type = message::Transport;
            using blocking_policy = policy::Blocking;
            using non_blocking_policy = policy::non::Blocking;
            using cache_type = policy::cache_type;


            Connector();
            ~Connector();

            Connector( Connector&&) noexcept = default;
            Connector& operator = ( Connector&&) noexcept = default;

            inline const Handle& handle() const { return m_handle;}
            inline Handle& handle() { return m_handle;}
            inline auto descriptor() const { return m_handle.socket().descriptor();}

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_handle, "handle");
            )

         private:
            Handle m_handle;
         };

         using Device = device::Inbound< Connector>;


         Device& device();
         inline Handle& handle() { return device().connector().handle();}
         inline strong::ipc::id ipc() { return device().connector().handle().ipc();}

      } // inbound

      namespace outbound
      {

         struct Connector
         {
            using transport_type = message::Transport;
            using blocking_policy = policy::Blocking;
            using non_blocking_policy = policy::non::Blocking;
            using complete_type = policy::complete_type;

            Connector() = default;
            Connector( strong::ipc::id destination);
            ~Connector() = default;

            inline const Address& destination() const { return m_destination;}
            inline const Socket& socket() const { return native::detail::outbound::socket();}

            inline void reconnect() const { throw; }
            inline void clear() { m_destination = Address{ strong::ipc::id{}};}

            inline friend std::ostream& operator << ( std::ostream& out, const Connector& rhs) 
            { 
               return out << "{ destination: " << rhs.m_destination << '}'; 
            }

         protected:
            Address m_destination;
         };

         template< typename S>
         using basic_device = device::Outbound< Connector, S>;

         using Device = device::Outbound< Connector>;

      } // outbound


      template< typename D, typename M, typename Device = inbound::Device>
      auto call(
            D&& destination,
            M&& message,
            Device& device = inbound::device())
      {
         return device::call( std::forward< D>( destination), std::forward< M>( message), device);
      }


      bool remove( strong::ipc::id id);
      bool remove( const process::Handle& owner);

      bool exists( strong::ipc::id id);

   } // common::communication::ipc

   namespace common::communication::device
   {
      template<>
      struct customization_point< strong::ipc::id>
      {
         using non_blocking_policy = ipc::policy::non::Blocking;
         using blocking_policy = ipc::policy::Blocking;

         template< typename... Ts>
         static auto send( strong::ipc::id ipc, Ts&&... ts) 
         {
            return device::send( ipc::outbound::Device{ ipc}, std::forward< Ts>( ts)...);
         }
      };
   } // common::communication::device

   
} // casual