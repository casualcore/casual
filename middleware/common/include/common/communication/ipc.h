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

#include <filesystem>

#include <sys/un.h>

namespace casual
{
   namespace common::communication::ipc
   {
      struct Handle
      {

         explicit Handle( Socket&& socket, strong::ipc::id ipc);
         Handle( Handle&& other) noexcept;
         Handle& operator = ( Handle&& other) noexcept;
         ~Handle();

         inline const Socket& socket() const { return m_socket;}
         inline strong::ipc::id ipc() const { return m_ipc;}
         inline strong::ipc::descriptor::id descriptor() const noexcept { return strong::ipc::descriptor::id{ m_socket.descriptor().underlying()};}

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

      //! @returns the path to the ipc "device"
      std::filesystem::path path( const strong::ipc::id& id);

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
            namespace create::domain
            {
               Socket socket();
            } // create::domain

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
         using cache_type = std::vector< message::Complete>;
         using cache_range_type = range::type_t< cache_type>;

         namespace blocking
         {
            cache_range_type receive( Handle& handle, cache_type& cache);
            strong::correlation::id send( const Socket& socket, const Address& destination, const message::Complete& complete);
         } // blocking


         struct Blocking
         {

            template< typename Connector>
            cache_range_type receive( Connector&& connector, cache_type& cache)
            {
               return policy::blocking::receive( connector.handle(), cache);
            }

            template< typename Connector>
            auto send( Connector&& connector, const message::Complete& complete)
            {
               return policy::blocking::send( connector.socket(), connector.destination(), complete);
            }
         };

         namespace non
         {
            namespace blocking
            {
               cache_range_type receive( Handle& handle, cache_type& cache);
               strong::correlation::id send( const Socket& socket, const Address& destination, const message::Complete& complete);
            } // blocking

            struct Blocking
            {
               template< typename Connector>
               cache_range_type receive( Connector&& connector, cache_type& cache)
               {
                  return policy::non::blocking::receive( connector.handle(), cache);
               }

               template< typename Connector>
               auto send( Connector&& connector, const message::Complete& complete)
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
            inline auto descriptor() const { return m_handle.descriptor();}

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
            using complete_type = message::Complete;

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

      namespace partial
      { 
         struct Destination
         {
            inline Destination( const strong::ipc::id& ipc)
               : m_ipc{ ipc}, m_socket{ ipc::native::detail::create::domain::socket()}, m_address{ ipc} {}

            inline auto& ipc() const noexcept { return m_ipc;}
            inline auto& socket() const noexcept { return m_socket;}
            inline auto& address() const noexcept { return m_address;}
            
            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE_NAME( m_ipc, "ipc");
               CASUAL_SERIALIZE_NAME( m_socket, "socket");
               CASUAL_SERIALIZE_NAME( m_address, "address");
            )

         private:
            strong::ipc::id m_ipc;
            Socket m_socket;
            Address m_address;
         }; 

         //! tries to send as much as possible of whats left in the complete send message
         //! @returns true if the complete send message
         bool send( const Destination& destination, message::complete::Send& complete);
         
      } // partial

      //! sends the `message` and receive the the reply.
      //! @returns the reply (_reverse type_ of message)
      //! @note blocking (of course)
      template< typename D, typename M, typename Device = inbound::Device>
      auto call( D&& destination, M&& message, Device& device = inbound::device())
         -> decltype( device::call( std::forward< D>( destination), std::forward< M>( message), device))
      {
         return device::call( std::forward< D>( destination), std::forward< M>( message), device);
      }

      //! @returns the received message of type `R`
      template< typename R, typename... Ts>
      auto receive( Ts&&... ts) -> decltype( device::receive< R>( inbound::device(), std::forward< Ts>( ts)...))
      {
         return device::receive< R>( inbound::device(), std::forward< Ts>( ts)...);
      }

      namespace non::blocking
      {
         //! @returns the received message of type `R`
         template< typename R, typename... Ts>
         auto receive( Ts&&... ts) -> decltype( device::non::blocking::receive< R>( inbound::device(), std::forward< Ts>( ts)...))
         {
            return device::non::blocking::receive< R>( inbound::device(), std::forward< Ts>( ts)...);
         }

      } // non::blocking


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