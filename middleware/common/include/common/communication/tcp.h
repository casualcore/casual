//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/communication/socket.h"
#include "common/communication/message.h"
#include "common/communication/device.h"
#include "common/serialize/native/network.h"
#include "common/pimpl.h"

#include "casual/platform.h"
#include "common/flag.h"
#include "common/strong/id.h"


#include <string>
#include <string_view>

namespace casual
{
   namespace common::communication::tcp
   {
      struct Address
      {
         Address() = default;
         Address( std::string address) : m_address{ std::move( address)} {}

         inline operator const std::string&() const { return m_address;}

         std::string_view host() const;
         std::string_view port() const;

         inline friend std::ostream& operator << ( std::ostream& out, const Address& value) { return out << value.m_address;}

         CASUAL_FORWARD_SERIALIZE( m_address)

      private:
         std::string m_address;
      };

      namespace socket
      {
         namespace address
         {
            //! @return The address to which the socket is bound to on local host
            Address host( strong::socket::id descriptor);
            Address host( const Socket& socket);

            //! @return The address of the peer connected to the socket
            Address peer( strong::socket::id descriptor);
            Address peer( const Socket& socket);

         } // address

         //! creates a socket, binds it to `address` and let the OS listen to the socket
         //! @param address 
         //! @return socket prepared to use with accept
         Socket listen( const Address& address);

         //! performs blocking accept on the `listener`
         //! @param listener 
         //! @return the connected socket
         Socket accept( const Socket& listener);

      } // socket

      Socket connect( const Address& address);

      namespace non::blocking
      {
         Socket connect( const Address& address);
      } // non::blocking

      namespace retry
      {
         //! Keeps trying to connect
         //!
         //! @param address to connect to
         //! @param sleep sleep pattern
         //! @return created socket
         Socket connect( const Address& address, process::pattern::Sleep sleep);
      } // retry


      class Listener
      {
      public:
         Listener( Address address);

         Socket operator() () const;

      private:
         Socket m_listener;
      };


      // Forwards
      struct Connector;

      namespace message
      {
         struct Policy
         {
            static constexpr platform::size::type message_size() { return platform::tcp::message::size;}
            static constexpr platform::size::type header_size( platform::size::type header_size, platform::size::type type_size) { return header_size + type_size;}
         };

      } // message


      namespace native
      {
         enum class Flag : long
         {
            non_blocking = platform::flag::value( platform::flag::tcp::no_wait)
         };

         Uuid send( const Socket& socket, const communication::message::Complete& complete, common::Flags< Flag> flags);
         communication::message::Complete receive( const Socket& socket, common::Flags< Flag> flags);

      } // native


      namespace policy
      {
         using cache_type = device::inbound::cache_type;
         using cache_range_type =  device::inbound::cache_range_type;


         struct basic_blocking
         {
            cache_range_type receive( const Connector& tcp, cache_type& cache);
            Uuid send( const Connector& tcp, const communication::message::Complete& complete);
         };

         using Blocking = basic_blocking;

         namespace non
         {
            struct basic_blocking
            {
               cache_range_type receive( const Connector& tcp, cache_type& cache);
               Uuid send( const Connector& tcp, const communication::message::Complete& complete);
            };

            using Blocking = basic_blocking;
         } // non

      } // policy

      struct Connector
      {
         using handle_type = Socket;
         using blocking_policy = policy::Blocking;
         using non_blocking_policy = policy::non::Blocking;

         Connector() noexcept;
         Connector( Socket&& socket) noexcept;
         Connector( const Socket& socket);

         Connector( const Connector&) = delete;
         Connector& operator = ( const Connector&) = delete;

         Connector( Connector&&) = default;
         Connector& operator = ( Connector&&) = default;

         inline const handle_type& socket() const { return m_socket;};
         inline handle_type& socket() { return m_socket;};
         inline auto descriptor() const { return m_socket.descriptor();}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_socket, "socket");
         )

      private:
         handle_type m_socket;
      };

      using Duplex = communication::device::Duplex< Connector, serialize::native::binary::network::create::Writer>;


   } // common::communication::tcp

   namespace common::serialize::traits::is::network
   {
      template<>
      struct normalizing< communication::tcp::Duplex>: std::true_type {};
   
   } // common::serialize::traits::is::network

} // casual


