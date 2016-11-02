//!
//! casual 
//!

#include "common/communication/tcp.h"
#include "common/communication/log.h"

#include "common/error.h"

#include "common/trace.h"



#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>


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
                        void error( const decltype( common::error::last()) last_error)
                        {
                           switch( last_error)
                           {
                              case EPIPE:
                              case ECONNRESET:
                              case ENOTCONN:
                                 throw common::exception::communication::Unavailable( common::error::string());
                              case ENOMEM:
                                 throw common::exception::limit::Memory( common::error::string());
                              case EAGAIN:
#if EAGAIN != EWOULDBLOCK
                              case EWOULDBLOCK:
#endif
                                 throw common::exception::communication::no::Message{ common::error::string()};
                              case EINVAL:
                                 throw common::exception::invalid::Argument{ "invalid arguments"};
                              case ENOTSOCK:
                                 throw common::exception::invalid::Argument{ "bad socket"};
                              case EINTR:
                              {
                                 common::signal::handle();

                                 //
                                 // we got a signal we don't have a handle for
                                 // We fall through
                                 //

                              } // @fallthrough
                              default:
                              {
                                 throw std::system_error{ last_error, std::system_category()};
                              }
                           }
                        }

                        int result( int result)
                        {
                           if( result == -1)
                           {
                              check::error( common::error::last());
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
                              throw exception::communication::Unavailable( gai_strerror( result), CASUAL_NIP( result), CASUAL_NIP( address));
                           }
                        }

                        std::unique_ptr< struct addrinfo, std::function< void( struct addrinfo*)>> deleter( information, &freeaddrinfo);

                        for( const struct addrinfo* info = information; info; info = info->ai_next)
                        {
                           auto socket = adopt( ::socket( info->ai_family, info->ai_socktype, info->ai_protocol));

                           if( socket && binder( socket, *info))
                           {
                              return socket;
                           }
                        }


                        switch( common::error::last())
                        {
                           case ECONNREFUSED:
                              throw exception::communication::Refused{ "connection refused", CASUAL_NIP( address)};
                           default:
                           {
                              throw std::system_error{ common::error::last(), std::system_category()};
                           }
                        }
                     }

                     Socket connect( const Address& address)
                     {
                        Trace trace( "common::communication::tcp::local::socket::connect");

                        //
                        // We block all signals while we're doing one connect attempt
                        //
                        //common::signal::thread::scope::Block block;

                        return create( address,[]( const Socket& s, const addrinfo& info)
                              {
                                 Trace trace( "common::communication::tcp::local::socket::connect lambda");

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

                        return create( address,[]( const Socket& s, const addrinfo& info)
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
                                 const int value = 1;

                                 check::result(
                                       ::setsockopt( s.descriptor(), SOL_SOCKET, SO_REUSEADDR, &value, sizeof( value))
                                 );


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
                     throw exception::invalid::Argument{ "invalid address", CASUAL_NIP( address)};
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

                  Address peer( const descriptor_type descriptor)
                  {
                     struct sockaddr info{ };
                     socklen_t size = sizeof( info);

                     local::socket::check::result(
                        getpeername(
                           descriptor, &info, &size));

                     return local::socket::names( info, size);
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
                     common::error::handler();
                  }
               }
            }

            Socket::Socket( const Socket& other)
               : m_descriptor{ local::socket::duplicate( other.m_descriptor)}
            {
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
                     catch( const exception::communication::Refused&)
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

                     ssize_t send( const socket::descriptor_type descriptor, const void* const data, std::size_t const size, common::Flags< Flag> flags)
                     {
                        common::signal::handle();

                        return tcp::local::socket::check::result(
                              ::send( descriptor, data, size, flags.underlaying()));

                     }

                     ssize_t receive( const socket::descriptor_type descriptor, void* const data, const std::size_t size, common::Flags< Flag> flags)
                     {
                       log << "descriptor: " << descriptor << ", data: " << static_cast< void*>( data) << ", size: " << size << ", flags: " << flags << '\n';

                        common::signal::handle();

                        const auto bytes = tcp::local::socket::check::result(
                              ::recv( descriptor, data, size, flags.underlaying()));

                        if( bytes == 0)
                        {
                           //
                           // Fake an error-description
                           //
                           throw common::exception::communication::Unavailable( common::error::string( EPIPE));
                        }
                        return bytes;
                     }


                  } // <unnamed>
               } // local

               bool send( const Socket& socket, const message::Transport& transport, common::Flags< Flag> flags)
               {
                  Trace trace{ "tcp::native::send"};

                  const auto size = message::Transport::header_size  + transport.size();

                  auto first = &transport.message;
                  const auto last = first + size;

                  try
                  {
                     while( first != last)
                     {
                        const auto bytes = local::send( socket.descriptor(), first, std::distance( first, last), flags);

                        if( bytes > std::distance( first, last))
                        {
                           throw exception::Casual( "somehow more bytes was sent over the socket than requested");
                        }

                        first += bytes;
                     }
                     log << "tcp send ---> socket: " << socket << ", transport: " << transport << '\n';
                     return true;
                  }
                  catch( const exception::communication::no::Message&)
                  {
                     return false;
                  }
               }

               bool receive( const Socket& socket, message::Transport& transport, common::Flags< Flag> flags)
               {
                  Trace trace{ "tcp::native::receive"};

                  const auto first = reinterpret_cast< char*>( &transport.message);
                  auto current = first;


                  try
                  {

                     //
                     // First we try to read all that we can, but at least the header
                     //

                     const auto header_end = first + message::Transport::header_size;
                     auto current_end = first + message::Transport::message_max_size;

                     while( current != current_end)
                     {
                        const auto current_distance = std::distance( current, current_end);

                        const auto bytes = local::receive( socket.descriptor(), current, current_distance, flags);

                        if( bytes > current_distance)
                        {
                           throw exception::Casual( "somehow more bytes was received over the socket than requested");
                        }

                        current += bytes;

                        if( current >= header_end)
                        {
                           current_end = header_end + transport.message.header.count;
                        }
                     }

                     log << "tcp receive <---- socket: " << socket << " , transport: " << transport << '\n';

                     return true;
                  }
                  catch( const exception::communication::no::Message&)
                  {
                     return false;
                  }
               }

            } // native

            namespace policy
            {
               bool basic_blocking::receive( const inbound::Connector& tcp, message::Transport& transport)
               {
                  return native::receive( tcp.socket(), transport, {});
               }

               bool basic_blocking::send( const outbound::Connector& tcp, const message::Transport& transport)
               {
                  return native::send( tcp.socket(), transport, {});
               }

               namespace non
               {
                  bool basic_blocking::receive( const inbound::Connector& tcp, message::Transport& transport)
                  {
                     return native::receive( tcp.socket(), transport, native::Flag::non_blocking);
                  }

                  bool basic_blocking::send( const outbound::Connector& tcp, const message::Transport& transport)
                  {
                     return native::send( tcp.socket(), transport, native::Flag::non_blocking);
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
