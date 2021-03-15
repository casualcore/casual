//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "common/communication/tcp/message.h"
#include "common/communication/log.h"
#include "common/communication/socket.h"
#include "common/communication/device.h"

#include "common/serialize/native/network.h"

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


      namespace policy
      {
         using complete_type = message::Complete;
         using cache_type = std::vector< complete_type>;
         using cache_range_type = range::type_t< cache_type>;

         struct Blocking
         {
            cache_range_type receive( const Connector& tcp, cache_type& cache);
            Uuid send( const Connector& tcp, complete_type&& complete);
         };

         namespace non
         {
            struct Blocking
            {
               cache_range_type receive( const Connector& tcp, cache_type& cache);
               complete_type send( const Connector& tcp, complete_type&& complete);
            };

         } // non

      } // policy

      struct Connector
      {
         using blocking_policy = policy::Blocking;
         using non_blocking_policy = policy::non::Blocking;
         using cache_type = policy::cache_type;
         using complete_type = policy::complete_type;

         Connector() noexcept;
         Connector( Socket&& socket) noexcept;
         Connector( const Socket& socket);

         Connector( const Connector&) = delete;
         Connector& operator = ( const Connector&) = delete;

         Connector( Connector&&) = default;
         Connector& operator = ( Connector&&) = default;

         inline const Socket& socket() const { return m_socket;};
         inline Socket& socket() { return m_socket;};
         inline auto descriptor() const { return m_socket.descriptor();}

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE_NAME( m_socket, "socket");
         )

      private:
         Socket m_socket;
      };

      using Duplex = communication::device::Duplex< Connector>;

   } // common::communication::tcp


   namespace common::serialize::traits::is::network
   {
      template<>
      struct normalizing< communication::tcp::Duplex>: std::true_type {};
      
   } //common::serialize::traits::is::network

} // casual


