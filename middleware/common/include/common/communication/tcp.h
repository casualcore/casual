//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once



#include "common/communication/message.h"
#include "common/communication/device.h"
#include "common/marshal/network.h"

#include "common/platform.h"
#include "common/flag.h"


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

            class Socket;


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


               friend std::ostream& operator << ( std::ostream& out, const Address& value);

               std::string host;
               std::string port;
            };

            namespace socket
            {
               using descriptor_type = platform::tcp::descriptor::type;

               namespace address
               {

                  //!
                  //! @return The address to which the socket is bound to on local host
                  //!
                  Address host( descriptor_type descriptor);
                  Address host( const Socket& socket);

                  //!
                  //! @return The address of the peer connected to the socket
                  //!
                  Address peer( descriptor_type descriptor);
                  Address peer( const Socket& socket);

               } // address

            } // socket

            class Socket
            {
            public:

               using descriptor_type = socket::descriptor_type;

               enum class Option : int
               {
                  reuse_address = SO_REUSEADDR,
                  linger = SO_LINGER,
               };

               Socket() noexcept;
               ~Socket() noexcept;

               Socket( const Socket&);
               Socket& operator = ( const Socket&);

               Socket( Socket&&) noexcept;
               Socket& operator = ( Socket&&) noexcept;

               explicit operator bool () const noexcept;

               //!
               //! Releases the responsibility of the socket
               //!
               //! @return descriptor
               //!
               descriptor_type release() noexcept;

               descriptor_type descriptor() const noexcept;

               void linger( std::chrono::seconds time);


               template< typename V>
               void option( Option option, V&& value)
               {
                  return Socket::option( cast::underlying( option), &value, sizeof( V));
               }

               friend Socket adopt( socket::descriptor_type descriptor);
               friend std::ostream& operator << ( std::ostream& out, const Socket& value);


            private:
               Socket( descriptor_type descriptor) noexcept;

               void option( int optname, const void *optval, size_type optlen);

               descriptor_type m_descriptor = -1;
               move::Moved m_moved;

            };

            Socket adopt( socket::descriptor_type descriptor);

            //!
            //! Duplicates the descriptor
            //!
            //! @param descriptor to be duplicated
            //! @return socket that owns the descriptor
            //!
            Socket duplicate( socket::descriptor_type descriptor);

            Socket connect( const Address& address);

            namespace retry
            {
               //!
               //! Keeps trying to connect
               //!
               //! @param address to connect to
               //! @param sleep sleep pattern
               //! @return created socket
               //!
               Socket connect( const Address& address, process::pattern::Sleep sleep);
            } // retry


            class Listener
            {
            public:
               Listener( Address adress);

               Socket operator() () const;

            private:
               Socket m_listener;
            };




            //
            // Forwards
            //
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
               using handle_type = tcp::Socket;
               using blocking_policy = policy::Blocking;
               using non_blocking_policy = policy::non::Blocking;

               base_connector() noexcept;
               base_connector( tcp::Socket&& socket) noexcept;
               base_connector( const tcp::Socket& socket);

               base_connector( const base_connector&) = delete;
               base_connector& operator = ( const base_connector&) = delete;

               base_connector( base_connector&&) = default;
               base_connector& operator = ( base_connector&&) = default;

               const handle_type& socket() const;

               friend std::ostream& operator << ( std::ostream& out, const base_connector& rhs);

            private:
               handle_type m_socket;
            };

            namespace inbound
            {
               struct Connector : base_connector
               {
                  using base_connector::base_connector;
               };

               using Device = communication::inbound::Device< Connector, marshal::binary::network::create::Input>;

            } // inbound

            namespace outbound
            {
               struct Connector : base_connector
               {
                  using base_connector::base_connector;

                  inline void reconnect() const { throw; }
               };

               using Device = communication::outbound::Device< Connector,  marshal::binary::network::create::Output>;
            } // outbound


            namespace dispatch
            {
               using Handler =  typename inbound::Device::handler_type;
            } // dispatch

         } // tcp
      } // communication

      namespace marshal
      {
         template<>
         struct is_network_normalizing< communication::tcp::inbound::Device> : std::true_type {};

         template<>
         struct is_network_normalizing< communication::tcp::outbound::Device> : std::true_type {};

      } // marshal

   } // common
} // casual


