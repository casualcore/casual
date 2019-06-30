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

#include "common/platform.h"
#include "common/flag.h"
#include "common/strong/id.h"


#include <string>


namespace casual
{
   namespace common
   {

      namespace communication
      {
         namespace tcp
         {
            using size_type = platform::size::type;
            using descriptor_type = communication::socket::descriptor_type;

            using Socket = communication::Socket;


            struct Address
            {
               struct Host : std::string
               {
                  Host( std::string s) : std::string{ std::move( s)} {}
               };

               struct Port : public std::string
               {
                  Port( std::string s) : std::string{ std::move( s)} {}
                  Port( long port) : std::string{ std::to_string( port)} {}
               };

               Address( const std::string& address);
               Address( Port port);
               Address( Host host, Port port);

               inline operator std::string() const { return host + ':' + port;} 

               // for logging only
               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
               {
                  CASUAL_SERIALIZE( host);
                  CASUAL_SERIALIZE( port);
               })

               std::string host;
               std::string port;
            };

            namespace socket
            {
               namespace address
               {
                  //! @return The address to which the socket is bound to on local host
                  Address host( descriptor_type descriptor);
                  Address host( const Socket& socket);

                  //! @return The address of the peer connected to the socket
                  Address peer( descriptor_type descriptor);
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
            namespace inbound
            {
               struct Connector;
            } // inbound

            namespace outbound
            {
               struct Connector;
            } // inbound


            namespace message
            {
               struct Policy
               {
                  static constexpr size_type message_size() { return platform::tcp::message::size;}
                  static constexpr size_type header_size( size_type header_size, size_type type_size) { return header_size + type_size;}
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
               using cache_type = communication::inbound::cache_type;
               using cache_range_type =  communication::inbound::cache_range_type;


               struct basic_blocking
               {
                  cache_range_type receive( const inbound::Connector& tcp, cache_type& cache);
                  Uuid send( const outbound::Connector& tcp, const communication::message::Complete& complete);
               };

               using Blocking = basic_blocking;

               namespace non
               {
                  struct basic_blocking
                  {
                     cache_range_type receive( const inbound::Connector& tcp, cache_type& cache);
                     Uuid send( const outbound::Connector& tcp, const communication::message::Complete& complete);
                  };

                  using Blocking = basic_blocking;
               } // non

            } // policy

            struct base_connector
            {
               using handle_type = Socket;
               using blocking_policy = policy::Blocking;
               using non_blocking_policy = policy::non::Blocking;

               base_connector() noexcept;
               base_connector( Socket&& socket) noexcept;
               base_connector( const Socket& socket);

               base_connector( const base_connector&) = delete;
               base_connector& operator = ( const base_connector&) = delete;

               base_connector( base_connector&&) = default;
               base_connector& operator = ( base_connector&&) = default;

               inline const handle_type& socket() const { return m_socket;};
               inline handle_type& socket() { return m_socket;};
               inline auto descriptor() const { return m_socket.descriptor();}

               // for logging only
               CASUAL_CONST_CORRECT_SERIALIZE_WRITE(
               {
                  CASUAL_SERIALIZE_NAME( m_socket, "socket");
               })

            private:
               handle_type m_socket;
            };

            namespace inbound
            {
               struct Connector : base_connector
               {
                  using base_connector::base_connector;
               };

               using Device = communication::inbound::Device< Connector, serialize::native::binary::network::create::Input>;

            } // inbound

            namespace outbound
            {
               struct Connector : base_connector
               {
                  using base_connector::base_connector;

                  inline void reconnect() const { throw; }
               };

               using Device = communication::outbound::Device< Connector,  serialize::native::binary::network::create::Output>;

               namespace blocking 
               {
                  template< typename M> 
                  auto send( const Socket& socket, M&& message)
                  {
                     return native::send( socket, serialize::native::complete( std::forward< M>( message), serialize::native::binary::network::create::Output{}), {});
                  }
               } // blocking 
            } // outbound


            namespace dispatch
            {
               using Handler =  typename inbound::Device::handler_type;
            } // dispatch

         } // tcp
      } // communication

      namespace serialize
      {
         namespace traits
         {
            namespace is
            {
               namespace network
               {
                  template<>
                  struct normalizing< communication::tcp::inbound::Device>: std::true_type {};

                  template<>
                  struct normalizing< communication::tcp::outbound::Device>: std::true_type {};
               } // network
            } // is 
         } // traits
      } // serialize
   } // common
} // casual


