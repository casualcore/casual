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
#include <variant>

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
            Address host( strong::socket::id descriptor) noexcept;
            Address host( const Socket& socket) noexcept;

            //! @return The address of the peer connected to the socket
            Address peer( strong::socket::id descriptor) noexcept;
            Address peer( const Socket& socket) noexcept;

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
      
      //! @returns a connected socket if success, otherwise:
      //!   * if fatal, non recoverable, error -> an exception is raised
      //!   * on non fatal errors a 'nil' socket is returned.
      Socket connect( const Address& address);

      namespace non::blocking
      {
         struct Pending
         {
            inline explicit Pending( Socket socket) noexcept 
               : m_socket{ std::move( socket)} {}

            inline Socket socket() && noexcept { return std::exchange( m_socket, {});}

         private:
            Socket m_socket;
         };

         namespace error
         {
            //! answers if the errc is a tcp recoverable error, or not.
            bool recoverable( std::errc error) noexcept;  
         } // error

         //! @returns one of:
         //!  * a socket
         //!     * valid socket -> successful connection
         //!     * 'nil'socket -> a recoverable error, worth trying to connect again.
         //!  * a _pending_ socket, that will be completed some time later.
         //!  * a fatal error (std::system_error), no point trying again
         std::variant< Socket, Pending, std::system_error> connect( const Address& address) noexcept;   

      } // non::blocking

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
            cache_range_type receive( Connector& tcp, cache_type& cache);
            strong::correlation::id send( Connector& tcp, complete_type&& complete);
         };

         namespace non
         {
            struct Blocking
            {
               cache_range_type receive( Connector& tcp, cache_type& cache);
               strong::correlation::id send( Connector& tcp, complete_type& complete);
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


