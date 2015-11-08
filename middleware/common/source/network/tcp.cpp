/*
 * tcp.cpp
 *
 *  Created on: Feb 9, 2015
 *      Author: kristone
 */

#include "common/network/tcp.h"

#include "common/network/byteorder.h"

#include "common/platform.h"


#include <netdb.h>
#include <unistd.h>

/*
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
*/

#include <cerrno>
#include <cstring>
#include <climits>

#include <stdexcept>
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

            Socket::Socket( const int descriptor) : m_descriptor( descriptor) {}

            Socket::~Socket()
            {
               if( *this)
               {
                  if( ::close( m_descriptor))
                  {
                     std::cerr << std::strerror( errno) << std::endl;
                  }
               }
            }

            Socket::Socket( Socket&&) noexcept = default;
            Socket& Socket::operator = ( Socket&&) noexcept = default;

            Socket::operator bool () const noexcept
            {
               //return m_descriptor != -1;
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
                        if( size > SSIZE_MAX)
                        {
                           // TODO: throw
                        }

                        const auto bytes = ::write( descriptor, data, size);

                        if( bytes < 0)
                        {
                           switch( errno)
                           {
                           case EDQUOT:
                           case EIO:
                           case ENOSPC:
                              throw std::runtime_error( std::strerror( errno));
                           case EINTR: // TODO: Handle signal
                           case EPIPE: // TODO: Handle signal
                           default:
                              throw std::logic_error( std::strerror( errno));
                           }
                        }

                        if( size - bytes)
                        {
                           push( descriptor, static_cast<const char*>(data) + bytes, size - bytes);
                        }

                     }

                     void pull( const int descriptor, void* const data, const std::size_t size)
                     {
                        if( size > SSIZE_MAX)
                        {
                           // TODO: throw
                        }

                        const auto bytes = ::read( descriptor, data, size);

                        if( bytes < 0)
                        {
                           switch( errno)
                           {
                           case EIO:
                              throw std::runtime_error( std::strerror( errno));
                           case EINTR: // TODO: Handle signal
                           default:
                              throw std::logic_error( std::strerror( errno));
                           }

                        }

                        if( size - bytes)
                        {
                           if( bytes)
                           {
                              //
                              // If EOF (or perhaps signal) this will end up in an eternal loop
                              //
                              pull( descriptor, static_cast<char*>(data) + bytes, size - bytes);
                           }
                           else
                           {
                              throw std::runtime_error( "Connection closed");
                           }
                        }

                     }

                  } // session
               } // local
            } //


            Session::Session( const int descriptor) : m_socket( descriptor) {}
            Session::~Session() = default;

            Session::Session( Session&&) noexcept = default;
            Session& Session::operator = ( Session&&) noexcept = default;

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
                           std::cerr << std::strerror( errno) << std::endl;
                        }
                     }

                     ~cork()
                     {
                        const int state = 0;
                        if( setsockopt( descriptor, IPPROTO_TCP, TCP_CORK, &state, sizeof( state)))
                        {
                           std::cerr << std::strerror( errno) << std::endl;
                        }
                     }
                  };
               } // local
            } //
*/

            void Session::push( const platform::binary_type& data) const
            {
               const auto encoded = byteorder::encode<platform::binary_size_type>( data.size());

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
               //
               // First read number of bytes to read later
               //
               byteorder::type<platform::binary_size_type> encoded;
               local::session::pull( m_socket.descriptor(), &encoded, sizeof( encoded));

               //
               // Allocate a buffer according to the decoded size
               //
               data.resize( byteorder::decode<platform::binary_size_type>( encoded));

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
                        struct addrinfo hints{ 0 };

                        // IPV4 or IPV6 doesn't matter
                        hints.ai_family = PF_UNSPEC;
                        // TCP/IP
                        hints.ai_socktype = SOCK_STREAM;

                        struct addrinfo* information = nullptr;

                        if( const int result = getaddrinfo( host.c_str(), port.c_str(), &hints, &information))
                        {
                           std::cerr << result << std::endl;
                           throw std::runtime_error( gai_strerror( result));
                        }

                        std::unique_ptr<struct addrinfo,std::function<void(struct addrinfo*)>> deleter( information, &freeaddrinfo);


                        std::vector<std::decay<decltype( errno)>::type> errors;

                        for( const struct addrinfo* info = information; info; info = info->ai_next)
                        {
                           Socket socket( ::socket( info->ai_family, info->ai_socktype, info->ai_protocol));

                           if( socket)
                           {
                              const int result = ::connect( socket.descriptor(), info->ai_addr, info->ai_addrlen);

                              if( result != -1)
                              {
                                 return socket;
                              }
                           }

                           errors.push_back( errno);

                        }

                        if( errors.empty())
                        {
                           throw std::logic_error( "Failed to connect client");
                        }
                        else
                        {
                           switch( errors.back())
                           {
                           case ENETUNREACH:
                           case ECONNREFUSED:
                           case EHOSTDOWN:
                           case ETIMEDOUT:
                              throw std::runtime_error( std::strerror( errors.back()));
                           case EINTR: // TODO: Handle signal
                           default:
                              throw std::logic_error( std::strerror( errors.back()));
                           }
                        }

                     }

                  } // client

               } // local

            } //


            Client::Client( const std::string& host, const std::string& port)
               : m_socket( local::client::connect( host, port))
            {}

            Client::~Client() = default;


            Session Client::session() const
            {
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

                        struct addrinfo hints{ 0 };

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
                           throw std::runtime_error( gai_strerror( result));
                        }

                        std::unique_ptr<struct addrinfo,std::function<void(struct addrinfo*)>> deleter( information, &freeaddrinfo);


                        std::vector<std::decay<decltype( errno)>::type> errors;

                        for( const struct addrinfo* info = information; info; info = info->ai_next)
                        {
                           Socket socket( ::socket( info->ai_family, info->ai_socktype, info->ai_protocol));

                           if( socket)
                           {
                              const auto result = ::bind( socket.descriptor(), info->ai_addr, info->ai_addrlen);

                              if( result != -1)
                              {
                                 //
                                 // TODO: (perhaps) setsockopt
                                 //

                                 return socket;
                              }
                           }

                           errors.push_back( errno);

                        }

                        if( errors.empty())
                        {
                           throw std::runtime_error( "Failed to connect server");
                        }
                        else
                        {
                           throw std::runtime_error( std::strerror( errors.back()));
                        }

                     }

                  } // server

               } // local

            } //


            Server::Server( const std::string& port)
               : m_socket( local::server::connect( port))
            {
               //
               // Could (probably) be set to zero as well (in casual-context)
               //
               const int queuesize = 5;

               const auto result = ::listen( m_socket.descriptor(), queuesize);

               if( result < 0)
               {
                  throw std::runtime_error( std::strerror( errno));
               }
            }

            Server::~Server() = default;

            Session Server::session() const
            {
               return Session( ::accept( m_socket.descriptor(), nullptr, nullptr));
            }


         } // tcp

      } // network

   } // common

} // casual


