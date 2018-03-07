//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/communication/tcp.h"
#include "common/communication/log.h"

#include "common/exception/system.h"
#include "common/exception/handle.h"

#include "common/log.h"



#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>


namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace tcp
         {

            namespace local
            {
               namespace
               {

                  namespace socket
                  {

                     namespace check
                     {
                        void error( common::code::system last_error)
                        {
                           using system = common::code::system;
                           switch( last_error)
                           {
                              case system::interrupted:
                              {
                                 common::signal::handle();

                                 //
                                 // we got a signal we don't have a handle for
                                 // We fall through
                                 //

                              } // @fallthrough
                              default:
                              {
                                 exception::system::throw_from_errno();
                              }
                           }
                        }

                        int result( int result)
                        {
                           if( result == -1)
                           {
                              check::error( common::code::last::system::error());
                           }
                           return result;
                        }
                     } // check



                     enum class Flag
                     {
                        // AI_PASSIVE allows wildcard IP-address
                        // AI_ADDRCONFIG only return addresses if configured
                        passive = AI_PASSIVE,
                        address_config = AI_ADDRCONFIG,
                     };

                     namespace address
                     {
                        const char* null_if_empty( const std::string& content)
                        {
                           if( content.empty()) { return nullptr;}
                           return content.c_str();
                        }

                        const char* host( const Address& address)
                        {
                           return null_if_empty( address.host);
                        }

                        const char* port( const Address& address)
                        {
                           return null_if_empty( address.port);
                        }
                     }

                     template< typename F>
                     Socket create( const Address& address, F binder, Flags< Flag> flags = {})
                     {
                        Trace trace( "common::communication::tcp::local::socket::create");

                        struct addrinfo* information = nullptr;


                        //
                        // resolve the address
                        //
                        {
                           struct addrinfo hints{ };

                           // IPV4 or IPV6 doesn't matter
                           hints.ai_family = PF_UNSPEC;
                           // TCP/IP
                           hints.ai_socktype = SOCK_STREAM;

                           hints.ai_flags = flags.underlaying();

                           if( const int result = getaddrinfo( address::host( address), address::port( address), &hints, &information))
                           {
                              throw exception::system::invalid::Argument( string::compose( 
                                 gai_strerror( result), " address: ", address));
                           }
                        }

                        auto deleter = memory::guard( information, &freeaddrinfo);

                        for( const struct addrinfo* info = information; info; info = info->ai_next)
                        {
                           auto socket = adopt( ::socket( info->ai_family, info->ai_socktype, info->ai_protocol));

                           if( socket && binder( socket, *info))
                           {
                              return socket;
                           }
                        }


                        switch( common::code::last::system::error())
                        {
                           case common::code::system::connection_refused:
                              throw exception::system::communication::Refused( string::compose( address));
                           default:
                           {
                              exception::system::throw_from_errno();
                           }
                        }
                        // will never be reached...
                        return {};
                     }

                     Socket connect( const Address& address)
                     {
                        Trace trace( "common::communication::tcp::local::socket::connect");

                        //
                        // We block all signals while we're doing one connect attempt
                        //
                        //common::signal::thread::scope::Block block;

                        return create( address,[]( Socket& s, const addrinfo& info)
                              {
                                 Trace trace( "common::communication::tcp::local::socket::connect lambda");

                                 //
                                 // To avoid possible TIME_WAIT from previous
                                 // possible connections
                                 //
                                 //
                                 s.option( Socket::Option::reuse_address, 1);
                                 s.linger( std::chrono::seconds{ 1});

                                 return ::connect( s.descriptor(), info.ai_addr, info.ai_addrlen) != -1;
                              });
                     }

                     Socket local( const Address& address)
                     {
                        Trace trace( "common::communication::tcp::local::socket::local");

                        //
                        // We block all signals while we're trying to set up a listener...
                        //
                        //common::signal::thread::scope::Block block;

                        static const Flags< Flag> flags{ Flag::address_config, Flag::passive};

                        return create( address,[]( Socket& s, const addrinfo& info)
                              {
                                 Trace trace( "common::communication::tcp::local::socket::local lambda");

                                 //
                                 // To avoid possible TIME_WAIT from previous
                                 // possible connections
                                 //
                                 // This might get not get desired results though
                                 //
                                 // Checkout SO_LINGER as well
                                 //
                                 s.option( Socket::Option::reuse_address, 1);
                                 s.linger( std::chrono::seconds{ 1});

                                 return ::bind( s.descriptor(), info.ai_addr, info.ai_addrlen) != -1;
                              }, flags);
                     }


                     tcp::socket::descriptor_type duplicate( const tcp::socket::descriptor_type descriptor)
                     {
                        Trace trace( "common::communication::tcp::local::socket::duplicate");

                        //
                        // We block all signals while we're trying to duplicate the descriptor
                        //
                        //common::signal::thread::scope::Block block;

                        const auto copy = check::result( ::dup( descriptor));

                        log << "descriptors - original: "<< descriptor << " , copy:" << copy <<'\n';

                        return copy;
                     }

                     Address names( const struct sockaddr& info, const socklen_t size)
                     {
                        char host[ NI_MAXHOST];
                        char serv[ NI_MAXSERV];
                        //const int flags{ NI_NUMERICHOST | NI_NUMERICSERV};
                        const int flags{ };

                        check::result(
                           getnameinfo(
                              &info, size,
                              host, NI_MAXHOST,
                              serv, NI_MAXSERV,
                              flags));

                        return { Address::Host{ host}, Address::Port{ serv}};
                     }


                  } // socket

               } // <unnamed>

            } // local

            Address::Address( const std::string& address)
            {
               auto parts = common::string::split( address, ':');

               switch( parts.size())
               {
                  case 1:
                  {
                     port = std::move( parts.front());
                     break;
                  }
                  case 2:
                  {
                     host = std::move( parts[ 0]);
                     port = std::move( parts[ 1]);
                     break;
                  }
                  default:
                  {
                     throw exception::system::invalid::Argument{ string::compose( "invalid address: ", address)};
                  }
               }
            }

            Address::Address( Port port) : port{ std::move( port)}
            {

            }

            Address::Address( Host host, Port port) : host{ std::move( host)}, port{ std::move( port)}
            {

            }

            std::ostream& operator << ( std::ostream& out, const Address& value)
            {
               return out << "{ host: " << value.host << ", port: " << value.port << '}';
            }

            namespace socket
            {

               namespace address
               {
                  Address host( const descriptor_type descriptor)
                  {
                     struct sockaddr info{ };
                     socklen_t size = sizeof( info);

                     local::socket::check::result(
                        getsockname(
                           descriptor, &info, &size));

                     return local::socket::names( info, size);
                  }

                  Address host( const Socket& socket)
                  {
                     return host( socket.descriptor());
                  }

                  Address peer( const descriptor_type descriptor)
                  {
                     struct sockaddr info{ };
                     socklen_t size = sizeof( info);

                     local::socket::check::result(
                        getpeername(
                           descriptor, &info, &size));

                     return local::socket::names( info, size);
                  }

                  Address peer( const Socket& socket)
                  {
                     return peer( socket.descriptor());
                  }

               } // address

            } // socket


            Socket::Socket() noexcept = default;

            Socket::Socket( const Socket::descriptor_type descriptor) noexcept : m_descriptor( descriptor) {}

            Socket::~Socket() noexcept
            {
               if( *this)
               {
                  try
                  {  
                     //local::socket::check::result( ::shutdown( m_descriptor, SHUT_RDWR));
                     local::socket::check::result( ::close( m_descriptor));
                     log << "Socket::close - descriptor: " << m_descriptor << '\n';
                  }
                  catch( ...)
                  {
                     exception::handle();
                  }
               }
            }

            Socket::Socket( const Socket& other)
               : m_descriptor{ local::socket::duplicate( other.m_descriptor)}
            {
            }

            void Socket::linger( std::chrono::seconds time)
            {
               struct
               {
                   int l_onoff;
                   int l_linger;
               } linger{ 1, static_cast< int>( time.count())};

               option( Option::linger, linger);
            }

            void Socket::option( int optname, const void *optval, size_type optlen)
            {
               local::socket::check::result( ::setsockopt( descriptor(), SOL_SOCKET, optname, optval, optlen));
            }

            Socket& Socket::operator = ( const Socket& other)
            {
               Socket copy{ other};
               return *this = std::move( copy);
            }

            Socket::Socket( Socket&&) noexcept = default;
            Socket& Socket::operator =( Socket&&) noexcept = default;

            Socket::operator bool() const noexcept
            {
               return ! m_moved && m_descriptor != -1;
            }

            Socket::descriptor_type Socket::descriptor() const noexcept
            {
               return m_descriptor;
            }

            Socket::descriptor_type Socket::release() noexcept
            {
               log << "Socket::release - descriptor: " << m_descriptor << '\n';

               const auto descriptor = m_descriptor;
               m_descriptor = -1;
               return descriptor;
            }


            Socket adopt( const socket::descriptor_type descriptor)
            {
               int val = 1;
              ::setsockopt( descriptor, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
               return { descriptor};
            }

            Socket duplicate( const socket::descriptor_type descriptor)
            {
               return adopt( local::socket::duplicate( descriptor));
            }

            std::ostream& operator << ( std::ostream& out, const Socket& value)
            {
               return out << "{ descriptor: " << value.m_descriptor << '}';
            }


            Socket connect( const Address& address)
            {
               Trace trace( "common::communication::tcp::connect");

               return local::socket::connect( address);

            }

            namespace retry
            {
               Socket connect( const Address& address, process::pattern::Sleep sleep)
               {
                  Trace trace( "common::communication::tcp::retry::connect");

                  while( true)
                  {
                     try
                     {
                        return tcp::connect( address);
                     }
                     catch( const exception::system::communication::Refused&)
                     {
                        sleep();
                     }
                  }

               }
            } // retry


            Listener::Listener( Address address) : m_listener{ local::socket::local( address)}
            {
               Trace trace( "common::communication::tcp::Listener ctor");

               //
               // Could (probably) be set to zero as well (in casual-context)
               //
               const int queuesize = 5;

               local::socket::check::result( ::listen( m_listener.descriptor(), queuesize));
            }

            Socket Listener::operator() () const
            {
               Trace trace( "common::communication::tcp::Listener::operator()");

               common::signal::handle();

               return adopt(
                     local::socket::check::result(
                           ::accept( m_listener.descriptor(), nullptr, nullptr)));
            }



            namespace native
            {
               namespace local
               {
                  namespace
                  {

                     ssize_t send( const socket::descriptor_type descriptor, const void* const data, size_type const size, common::Flags< Flag> flags)
                     {
                        common::signal::handle();

                        return tcp::local::socket::check::result(
                              ::send( descriptor, data, size, flags.underlaying()));

                     }

                     char* receive(
                           const socket::descriptor_type descriptor,
                           char* first,
                           char* const last,
                           common::Flags< Flag> flags)
                     {
                        Trace trace{ "tcp::native::local::receive"};

                        log << "descriptor: " << descriptor << ", data: " << static_cast< void*>( first) << ", size: " << last - first << ", flags: " << flags << '\n';


                        while( first != last)
                        {
                           common::signal::handle();

                           const auto bytes = tcp::local::socket::check::result(
                                 ::recv( descriptor, first, last - first, flags.underlaying()));

                           if( bytes == 0)
                           {
                              //
                              // Fake an error-description
                              //
                              throw exception::system::communication::unavailable::Pipe{};
                           }

                           first += bytes;
                        }
                        return first;
                     }
                  } // <unnamed>
               } // local


               Uuid send( const Socket& socket, const communication::message::Complete& complete, common::Flags< Flag> flags)
               {
                  Trace trace{ "tcp::native::send"};


                  try
                  {

                     auto local_send = []( auto descriptor, auto first, auto last, auto flags){
                        while( first != last)
                        {
                           const auto bytes = local::send( descriptor, first, std::distance( first, last), flags);

                           if( bytes > std::distance( first, last))
                           {
                              throw exception::system::communication::Protocol( "somehow more bytes was sent over the socket than requested");
                           }

                           first += bytes;
                        }
                     };

                     //
                     // First we send the header
                     //
                     {
                        auto header = complete.header();

                        auto first = reinterpret_cast< const char*>( &header);
                        auto last = first + communication::message::complete::network::header::size();

                        local_send( socket.descriptor(), first, last, flags);
                     }

                     //
                     // Now we can send the payload
                     //
                     {
                        local_send( socket.descriptor(), complete.payload.data(), complete.payload.data() + complete.payload.size(), flags);
                     }

                     log << "tcp send ---> socket: " << socket << ", complete: " << complete << '\n';

                     return complete.correlation;
                  }
                  catch( const exception::system::communication::no::Message&)
                  {
                     return uuid::empty();
                  }
               }

               communication::message::Complete receive( const Socket& socket, common::Flags< Flag> flags)
               {
                  Trace trace{ "tcp::native::receive"};

                  try
                  {

                     communication::message::complete::network::Header header;

                     auto current = reinterpret_cast< char*>( &header);

                     //
                     // First we get the header
                     //
                     {
                        const auto header_end = current + communication::message::complete::network::header::size();

                        local::receive( socket.descriptor(), current, header_end, flags);

                        log << "header: " << header << '\n';
                     }

                     //
                     // Now we can get the payload
                     //

                     communication::message::Complete message{ header};

                     // make sure we always block when we wait for the payload.
                     local::receive( socket.descriptor(), message.payload.data(), message.payload.data() + message.payload.size(), flags - Flag::non_blocking);

                     log << "tcp receive <---- socket: " << socket << " , complete: " << message << '\n';

                     return message;
                  }
                  catch( const exception::system::communication::no::Message&)
                  {
                     return {};
                  }
               }

               namespace local
               {
                  namespace
                  {
                     policy::cache_range_type receive( const inbound::Connector& tcp, policy::cache_type& cache, common::Flags< Flag> flags)
                     {
                        auto message = native::receive( tcp.socket(), flags);

                        if( message)
                        {
                           cache.push_back( std::move( message));

                           return { std::end( cache) - 1, std::end( cache)};
                        }
                        return {};
                     }
                  } // <unnamed>
               } // local

            } // native

            namespace policy
            {

               cache_range_type basic_blocking::receive( const inbound::Connector& tcp, cache_type& cache)
               {
                  return native::local::receive( tcp, cache, {});
               }

               Uuid basic_blocking::send( const outbound::Connector& tcp, const communication::message::Complete& complete)
               {
                  return native::send( tcp.socket(), complete, {});
               }


               namespace non
               {
                  cache_range_type basic_blocking::receive( const inbound::Connector& tcp, cache_type& cache)
                  {
                     return native::local::receive( tcp, cache, native::Flag::non_blocking);
                  }

                  Uuid basic_blocking::send( const outbound::Connector& tcp, const communication::message::Complete& complete)
                  {
                     return native::send( tcp.socket(), complete, native::Flag::non_blocking);
                  }

               } // non

            } // policy

            base_connector::base_connector() noexcept = default;

            base_connector::base_connector( tcp::Socket&& socket) noexcept
                  : m_socket( std::move( socket))
            {

            }

            base_connector::base_connector( const tcp::Socket& socket)
             : m_socket{ socket}
            {

            }


            const base_connector::handle_type& base_connector::socket() const
            {
               return m_socket;
            }

            std::ostream& operator << ( std::ostream& out, const base_connector& rhs)
            {
               return out << "{ socket: " << rhs.m_socket << '}';
            }


         } // tcp
      } // communication
   } // common
} // casual
