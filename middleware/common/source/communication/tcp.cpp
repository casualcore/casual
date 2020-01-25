//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/communication/tcp.h"
#include "common/communication/log.h"
#include "common/communication/select.h"

#include "common/result.h"

#include "common/exception/system.h"
#include "common/exception/handle.h"

#include "common/log.h"

// posix
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

                     namespace option
                     {
                        struct no_delay
                        {
                           constexpr static auto level() { return IPPROTO_TCP;}
                           constexpr static auto option() { return TCP_NODELAY;}
                           constexpr static int value() { return 1;}
                        };

                     } // option


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

                        // resolve the address
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
                           auto socket = Socket{ 
                              descriptor_type{ ::socket( info->ai_family, info->ai_socktype, info->ai_protocol)}};

                           if( socket && binder( socket, *info))
                           {
                              socket.set( local::socket::option::no_delay{});
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

                        // We block all signals while we're doing one connect attempt
                        //common::signal::thread::scope::Block block;

                        return create( address,[]( Socket& s, const addrinfo& info)
                              {
                                 Trace trace( "common::communication::tcp::local::socket::connect lambda");

                                 // To avoid possible TIME_WAIT from previous
                                 // possible connections
                                 s.set( communication::socket::option::reuse_address< true>{});
                                 s.set( communication::socket::option::linger{ std::chrono::seconds{ 1}});

                                 return ::connect( s.descriptor().value(), info.ai_addr, info.ai_addrlen) != -1;
                              });
                     }


                     Socket local( const Address& address)
                     {
                        Trace trace( "common::communication::tcp::local::socket::local");

                        // We block all signals while we're trying to set up a listener...
                        //common::signal::thread::scope::Block block;

                        static const Flags< Flag> flags{ Flag::address_config, Flag::passive};

                        return create( address,[]( Socket& s, const addrinfo& info)
                        {
                           Trace trace( "common::communication::tcp::local::socket::local lambda");

                           // To avoid possible TIME_WAIT from previous
                           // possible connections
                           //
                           // This might get not get desired results though
                           //
                           // Checkout SO_LINGER as well
                           s.set( communication::socket::option::reuse_address< true>{});
                           s.set( communication::socket::option::linger{ std::chrono::seconds{ 1}});

                           return ::bind( s.descriptor().value(), info.ai_addr, info.ai_addrlen) != -1;
                        }, flags);
                     }



                     Address names( const struct sockaddr& info, const socklen_t size)
                     {
                        char host[ NI_MAXHOST];
                        char serv[ NI_MAXSERV];
                        //const int flags{ NI_NUMERICHOST | NI_NUMERICSERV};
                        const int flags{ };

                        posix::result(
                           getnameinfo(
                              &info, size,
                              host, NI_MAXHOST,
                              serv, NI_MAXSERV,
                              flags));

                        return { Address::Host{ host}, Address::Port{ serv}};
                     }

                     Socket accept( const descriptor_type descriptor)
                     {
                        Socket result{ descriptor_type{
                              posix::result( ::accept( descriptor.value(), nullptr, nullptr))}};
                        
                        result.set( socket::option::no_delay{});

                        return result;
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

            namespace socket
            {

               namespace address
               {
                  Address host( const descriptor_type descriptor)
                  {
                     struct sockaddr info{ };
                     socklen_t size = sizeof( info);

                     posix::result(
                        getsockname(
                           descriptor.value(), &info, &size));

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

                     posix::result(
                        getpeername(
                           descriptor.value(), &info, &size));

                     return local::socket::names( info, size);
                  }

                  Address peer( const Socket& socket)
                  {
                     return peer( socket.descriptor());
                  }

               } // address

               Socket listen( const Address& address)
               {
                  Trace trace( "common::communication::tcp::socket::listen");

                  auto result = local::socket::local( address);

                  // queuesize could (probably) be set to zero as well (in casual-context)
                  posix::result( ::listen( result.descriptor().value(), platform::tcp::listen::backlog));

                  return result;
               }

               Socket accept( const Socket& listener)
               {
                  Trace trace( "common::communication::tcp::socket::accept");

                  return local::socket::accept( listener.descriptor());
               }

            } // socket



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

                  do
                  {
                     try
                     {
                        return tcp::connect( address);
                     }
                     catch( const exception::system::communication::Refused&)
                     {
                        // no-op - we go to sleep
                     }
                  }
                  while( sleep());

                  return {};
               }
            } // retry


            Listener::Listener( Address address) : m_listener{ socket::listen( address)}
            {
            }

            Socket Listener::operator() () const
            {
               Trace trace( "common::communication::tcp::Listener::operator()");

               // make sure we "safely" block and wait for a connection
               communication::select::block::read( m_listener.descriptor());

               return socket::accept( m_listener);
            }



            namespace native
            {
               namespace local
               {
                  namespace
                  {

                     ssize_t send( const descriptor_type descriptor, const void* const data, size_type const size, common::Flags< Flag> flags)
                     {
                        return posix::result( 
                           ::send( descriptor.value(), data, size, flags.underlaying()));
                     }

                     char* receive(
                           const descriptor_type descriptor,
                           char* first,
                           char* const last,
                           common::Flags< Flag> flags)
                     {
                        Trace trace{ "tcp::native::local::receive"};

                        common::log::line( log, "descriptor: ", descriptor, ", data: ", static_cast< void*>( first), ", size: ", last - first, ", flags: ", flags);


                        while( first != last)
                        {
                           const auto bytes = posix::result(
                                 ::recv( descriptor.value(), first, last - first, flags.underlaying()));

                           if( bytes == 0)
                           {
                              // Fake an error-description
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

                     // First we send the header
                     {
                        auto header = complete.header();

                        auto first = reinterpret_cast< const char*>( &header);
                        auto last = first + communication::message::complete::network::header::size();

                        local_send( socket.descriptor(), first, last, flags);
                     }

                     // Now we can send the payload
                     {
                        local_send( socket.descriptor(), complete.payload.data(), complete.payload.data() + complete.payload.size(), flags);
                     }

                     log::line( log, "tcp send ---> socket: ", socket, ", complete: ", complete);

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

                     // First we get the header
                     {
                        const auto header_end = current + communication::message::complete::network::header::size();

                        local::receive( socket.descriptor(), current, header_end, flags);

                        common::log::line( verbose::log, "header: ", header);

                     }

                     // Now we can get the payload

                     communication::message::Complete message{ header};

                     // make sure we always block when we wait for the payload.
                     local::receive( socket.descriptor(), message.payload.data(), message.payload.data() + message.payload.size(), flags - Flag::non_blocking);

                     log::line( log, "tcp receive <---- socket: ", socket, " , complete: ", message);

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

                           return policy::cache_range_type{ std::end( cache) - 1, std::end( cache)};
                        }
                        return policy::cache_range_type{};
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

            base_connector::base_connector( Socket&& socket) noexcept
                  : m_socket( std::move( socket))
            {

            }

            base_connector::base_connector( const Socket& socket)
             : m_socket{ socket}
            {

            }

         } // tcp
      } // communication
   } // common
} // casual
