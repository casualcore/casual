/*
 * tcp.cpp
 *
 *  Created on: Feb 9, 2015
 *      Author: kristone
 */

#include "common/network/tcp.h"

#include "common/platform.h"
#include "common/trace.h"
#include "common/error.h"
#include "common/exception.h"

#include "common/network/byteorder.h"

#include <netdb.h>
#include <unistd.h>


 #include <sys/types.h>
 #include <sys/socket.h>
/*
 #include <netinet/tcp.h>
 */

#include <climits>

#include <vector>
#include <memory>
#include <type_traits>

#include <iostream>

namespace casual
{
   namespace common
   {
      namespace network
      {

         namespace tcp
         {
            Socket::Socket( const int descriptor) noexcept : m_descriptor( descriptor) {}

            Socket::~Socket() noexcept
            {
               if( *this)
               {
                  if( ::close( m_descriptor))
                  {
                     std::cerr << error::string() << std::endl;
                  }
               }
            }

            Socket::Socket( Socket&&) noexcept = default;
            Socket& Socket::operator =( Socket&&) noexcept = default;

            Socket::operator bool() const noexcept
            {
               return ! m_moved && m_descriptor != -1;
            }

            int Socket::descriptor() const noexcept
            {
               return m_descriptor;
            }

            namespace
            {
               namespace local
               {
                  namespace session
                  {
                     void push( const int descriptor, const void* const data, const std::size_t size)
                     {

                        //
                        // This is no optimization, but because recv is not specified
                        // how to behave with zero-size-messages (see pull)
                        //
                        if( ! size) return;


                        if( size > SSIZE_MAX)
                        {
                           throw exception::invalid::Argument( "max size exceeded", CASUAL_NIP( size));
                        }

                        const auto bytes = ::send( descriptor, data, size, platform::flag::value( platform::flag::msg::no_signal));

                        if( bytes < 0)
                        {
                           switch( errno)
                           {
                           case EPIPE:
                           case ECONNRESET:
                           case ENOTCONN:
                              throw exception::network::Unavailable( error::string());
                           case ENOMEM:
                              throw exception::limit::Memory( error::string());
                           default:
                              //
                              // Handle all these as programming defects (until other need is proven)
                              //
                              throw exception::Casual( error::string());
                           }

                        }

                        if( size - bytes)
                        {
                           push( descriptor, static_cast< const char*>( data) + bytes, size - bytes);
                        }

                     }

                     void pull( const int descriptor, void* const data, const std::size_t size)
                     {
                        //
                        // This is no optimization, but because recv is not specified
                        // how to behave with zero-size-messages (see push)
                        //
                        if( ! size) return;


                        if( size > SSIZE_MAX)
                        {
                           throw exception::invalid::Argument( "size too big", CASUAL_NIP( size));
                        }

                        const constexpr auto flags = 0;
                        const auto bytes = ::recv( descriptor, data, size, flags);

                        if( bytes < 0)
                        {
                           switch( errno)
                           {
                           case ENOMEM:
                              throw exception::limit::Memory( error::string());
                           default:
                              //
                              // Handle all these as programming defects (until other need is proven)
                              //
                              throw exception::Casual( error::string());
                           }

                        }

                        if( bytes == 0)
                        {
                           //
                           // Fake an error-description
                           //
                           throw exception::network::Unavailable( error::string( EPIPE));
                        }

                        if( size - bytes)
                        {
                           //
                           // If EOF (or perhaps signal) this will end up in an eternal loop
                           //
                           pull( descriptor, static_cast< char*>( data) + bytes, size - bytes);
                        }

                     }

                  } // session
               } // local
            } //

            Session::Session( const int descriptor) noexcept : m_socket( descriptor) {}
            Session::~Session() noexcept = default;

            Session::Session( Session&&) noexcept = default;
            Session& Session::operator =( Session&&) noexcept = default;

            /*
             namespace
             {
                namespace local
                {
                   struct cork
                   {
                      const int descriptor;
                      cork( const int descriptor) : descriptor( descriptor)
                      {
                         const int state = 1;
                         if( setsockopt( descriptor, IPPROTO_TCP, TCP_CORK, &state, sizeof( state)))
                         {
                            std::cerr << error::string() << std::endl;
                         }
                      }

                      ~cork()
                      {
                         const int state = 0;
                         if( setsockopt( descriptor, IPPROTO_TCP, TCP_CORK, &state, sizeof( state)))
                         {
                            std::cerr << error::string() << std::endl;
                         }
                      }
                   };
                } // local
             } //
             */

            void Session::push( const platform::binary_type& data) const
            {
               const trace::Scope trace( "Session::push");

               const auto encoded = byteorder::encode< platform::binary_size_type>( data.size());

               //const local::cork cork( m_socket.descriptor());

               //
               // First write number of bytes to write later
               //
               local::session::push( m_socket.descriptor(), &encoded, sizeof( encoded));

               //
               // Write the bytes
               //
               local::session::push( m_socket.descriptor(), data.data(), data.size());

            }

            void Session::pull( platform::binary_type& data) const
            {
               const trace::Scope trace( "Session::pull");

               //
               // First read number of bytes to read later
               //
               byteorder::type< platform::binary_size_type> encoded;
               local::session::pull( m_socket.descriptor(), &encoded, sizeof( encoded));

               //
               // Allocate a buffer according to the decoded size
               //
               data.resize( byteorder::decode< platform::binary_size_type>( encoded));

               //
               // Read the bytes
               //
               local::session::pull( m_socket.descriptor(), data.data(), data.size());
            }

            platform::binary_type Session::pull() const
            {
               platform::binary_type result;
               pull( result);
               return result;
            }

            namespace
            {
               namespace local
               {
                  namespace client
                  {
                     Socket connect( const std::string& host, const std::string& port)
                     {
                        const trace::Scope trace( "client::connect");

                        struct addrinfo hints{ };

                        // IPV4 or IPV6 doesn't matter
                        hints.ai_family = PF_UNSPEC;
                        // TCP/IP
                        hints.ai_socktype = SOCK_STREAM;

                        struct addrinfo* information = nullptr;

                        if( const int result = getaddrinfo( host.c_str(), port.c_str(), &hints, &information))
                        {
                           throw exception::network::Unavailable( gai_strerror( result), CASUAL_NIP( result), CASUAL_NIP( host), CASUAL_NIP( port));
                        }

                        std::unique_ptr< struct addrinfo, std::function< void( struct addrinfo*)>> deleter( information, &freeaddrinfo);

                        //
                        // Add (at least) one (fake) error-code
                        //
                        std::vector < std::decay< decltype( errno)>::type > errors{ ENETUNREACH};

                        for( const struct addrinfo* info = information; info; info = info->ai_next)
                        {
                           Socket socket( ::socket( info->ai_family, info->ai_socktype, info->ai_protocol));

                           if( socket)
                           {
                              if( ::connect( socket.descriptor(), info->ai_addr, info->ai_addrlen) != -1)
                              {
                                 return socket;
                              }
                           }

                           errors.push_back( errno);

                        }

                        //
                        // TODO: switch( errno) and throw special exceptions
                        //

                        throw exception::network::Unavailable(
                           error::string( errors.back()),
                           CASUAL_NIP( host),
                           CASUAL_NIP( port));

                     }

                  } // client

               } // local

            } //

            Client::Client( const std::string& host, const std::string& port)
            : m_socket( local::client::connect( host, port)) { }

            Client::~Client() = default;

            Session Client::session() const
            {
               const trace::Scope trace( "Client::session");

               return Session( ::dup( m_socket.descriptor()));
            }

            namespace
            {
               namespace local
               {
                  namespace server
                  {

                     Socket connect( const std::string& port)
                     {
                        const trace::Scope trace( "server::connect");

                        struct addrinfo hints{ };

                        // IPV4 or IPV6 doesn't matter
                        hints.ai_family = PF_UNSPEC;
                        // TCP/IP
                        hints.ai_socktype = SOCK_STREAM;
                        // AI_PASSIVE allows wildcard IP-address
                        // AI_ADDRCONFIG only return addresses if configured
                        hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;

                        struct addrinfo* information = nullptr;

                        if( const int result = getaddrinfo( nullptr, port.c_str(), &hints, &information))
                        {
                           throw exception::network::Unavailable( gai_strerror( result), CASUAL_NIP( result), CASUAL_NIP( port));
                        }

                        std::unique_ptr< struct addrinfo, std::function< void( struct addrinfo*)>> deleter( information, &freeaddrinfo);

                        //
                        // Add (at least) one (fake) error-code
                        //
                        std::vector < std::decay< decltype( errno)>::type > errors{ ENETUNREACH};

                        for( const struct addrinfo* info = information; info; info = info->ai_next)
                        {
                           Socket socket( ::socket( info->ai_family, info->ai_socktype, info->ai_protocol));

                           if( socket)
                           {
                              //
                              // To avoid possible TIME_WAIT from previous
                              // possible connections
                              //
                              // This might get not get desired results though
                              //
                              // Checkout SO_LINGER as well
                              //
                              const int value = 1;
                              if( ::setsockopt( socket.descriptor(), SOL_SOCKET, SO_REUSEADDR, &value, sizeof( value)) < 0)
                              {
                                 throw exception::Casual( error::string());
                              }

                              if( ::bind( socket.descriptor(), info->ai_addr, info->ai_addrlen) != -1)
                              {
                                 return socket;
                              }
                           }

                           errors.push_back( errno);
                        }

                        //
                        // TODO: switch( errno) and throw special exceptions
                        //

                        throw exception::network::Unavailable(
                           error::string( errors.back()),
                           CASUAL_NIP( port));

                     }

                  } // server

               } // local

            } //

            Server::Server( const std::string& port) : m_socket( local::server::connect( port))
            {
               const trace::Scope trace( "Server::Server");

               //
               // Could (probably) be set to zero as well (in casual-context)
               //
               const int queuesize = 5;

               const auto result = ::listen( m_socket.descriptor(), queuesize);

               if( result < 0)
               {
                  throw exception::network::Unavailable( error::string(), CASUAL_NIP( port));
               }
            }

            Server::~Server() = default;

            Session Server::session() const
            {
               const trace::Scope trace( "Server::session");

               // Skip error-handling (until proven needed)
               return Session{ ::accept( m_socket.descriptor(), nullptr, nullptr)};
            }

         } // tcp

      } // network

   } // common

} // casual

