//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/communication/tcp.h"
#include "common/communication/log.h"
#include "common/communication/select.h"

#include "common/result.h"

//#include "common/exception/handle.h"

#include "common/code/convert.h"
#include "common/code/raise.h"
#include "common/code/system.h"
#include "common/code/casual.h"

#include "common/log.h"

// posix
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/tcp.h>



std::ostream& operator << ( std::ostream& out, const struct addrinfo& value)
{
   out << "{ protocol: " << value.ai_protocol
      << ", family: " << value.ai_family;
   
   if( value.ai_canonname)
      out << ", canonname: " << value.ai_canonname;

   return out << '}';
}


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

                     namespace outcome
                     {
                        enum class Create : short
                        {
                           ok,
                           pending,
                           error,
                        };

                     } // outcome


                     enum class Flag
                     {
                        // AI_PASSIVE allows wildcard IP-address
                        // AI_ADDRCONFIG only return addresses if configured
                        passive = AI_PASSIVE,
                        address_config = AI_ADDRCONFIG,
                        canonical = AI_CANONNAME,
                     };

                     namespace address
                     {
                        struct Native 
                        {
                           explicit Native( const tcp::Address& address, Flags< Flag> flags = {})
                           {
                              Trace trace( "common::communication::tcp::local::socket::address::Native::Native");
                              log::line( verbose::log, "address: ", address, ", flags: ", flags);

                              struct addrinfo hints{};

                              // IPV4 or IPV6 doesn't matter
                              hints.ai_family = PF_UNSPEC;
                              // TCP/IP
                              hints.ai_socktype = SOCK_STREAM;

                              flags |= Flag::canonical;
                              hints.ai_flags = flags.underlaying();

                              std::string host{ address.host()};
                              std::string port{ address.port()};

                              if( const int result = ::getaddrinfo( host.data(), port.data(), &hints, &information.value))
                                 code::raise::log( code::casual::invalid_argument, gai_strerror( result), " address: ", address);
                           }

                           Native() = default;

                           ~Native()
                           {
                              if( information)
                                 freeaddrinfo( information.value);
                           }

                           Native( Native&&) noexcept = default;
                           Native& operator = ( Native&&) noexcept = default;

                           struct iterator
                           {
                              iterator() = default;
                              iterator( const addrinfo* data) : data{ data} {}

                              auto& operator * () const { return *data;}
                              auto operator -> () const { return data;}

                              iterator& operator ++ () { data = data->ai_next; return *this;}
                              iterator operator ++ ( int) { iterator result{ data}; data = data->ai_next; return result;}

                              friend bool operator == ( iterator lhs, iterator rhs) { return lhs.data == rhs.data;}
                              friend bool operator != ( iterator lhs, iterator rhs) { return ! ( lhs == rhs);}
                              const addrinfo* data = nullptr;
                           };

                           auto begin() { return iterator{ information.value};}
                           auto end() { return iterator{};}
                           auto begin() const { return iterator{ information.value};}
                           auto end() const { return iterator{};}

                           bool empty() const noexcept { return information.value == nullptr;}

                           common::move::Pointer< addrinfo> information;
                        };

                     } // address

                     template< typename F>
                     Socket create( const Address& address, F binder, Flags< Flag> flags = {})
                     {
                        Trace trace( "common::communication::tcp::local::socket::create");

                        address::Native native{ address, flags}; 
                        log::line( verbose::log, "native: ", native);

                        for( auto& info : native)
                        {
                           auto socket = Socket{ 
                              strong::socket::id{ ::socket( info.ai_family, info.ai_socktype, info.ai_protocol)}};

                           if( socket && binder( socket, info))
                           {
                              socket.set( local::socket::option::no_delay{});

                              // make sure we honour keepalive
                              socket.set( communication::socket::option::keepalive< true>{});
                              return socket; 
                           }
                        }

                        code::raise::log( code::convert::to::casual( code::system::last::error()), "address: ", address);
                     }

                     Socket connect( const Address& address)
                     {
                        Trace trace( "common::communication::tcp::local::socket::connect");

                        // We block all signals while we're doing one connect attempt
                        //common::signal::thread::scope::Block block;

                        return create( address,[]( Socket& socket, const addrinfo& info)
                        {
                           Trace trace( "common::communication::tcp::local::socket::connect lambda");

                           // To avoid possible TIME_WAIT from previous possible connections
                           socket.set( communication::socket::option::reuse_address< true>{});
                           socket.set( communication::socket::option::linger{ std::chrono::seconds{ 1}});

                           if( ::connect( socket.descriptor().value(), info.ai_addr, info.ai_addrlen) == 0)
                              return true;

                           log::line( verbose::log, code::system::last::error(), ", socket: ", socket, ", info: ", info);
                           return false;
                        });
                     }

                     auto local( const Address& address)
                     {
                        Trace trace( "common::communication::tcp::local::socket::local");

                        // We block all signals while we're trying to set up a listener...
                        //common::signal::thread::scope::Block block;

                        static const Flags< Flag> flags{ Flag::address_config, Flag::passive};

                        return create( address,[]( Socket& socket, const addrinfo& info)
                        {
                           Trace trace( "common::communication::tcp::local::socket::local lambda");

                           // To avoid possible TIME_WAIT from previous
                           // possible connections
                           //
                           // This might get not get desired results though
                           //
                           // Checkout SO_LINGER as well
                           socket.set( communication::socket::option::reuse_address< true>{});
                           socket.set( communication::socket::option::linger{ std::chrono::seconds{ 1}});

                           return ::bind( socket.descriptor().value(), info.ai_addr, info.ai_addrlen) != -1;
                        }, flags);
                     }

                     Address names( const struct sockaddr& info, const socklen_t size)
                     {
                        char host[ NI_MAXHOST];
                        char serv[ NI_MAXSERV];
                        const int flags{ NI_NUMERICHOST | NI_NUMERICSERV};

                        posix::result(
                           getnameinfo(
                              &info, size,
                              host, NI_MAXHOST,
                              serv, NI_MAXSERV,
                              flags));

                        return { string::compose( host, ':', serv)};
                     }

                     Socket accept( const strong::socket::id descriptor)
                     {
                        auto result = ::accept( descriptor.value(), nullptr, nullptr);

                        if( result == -1)
                        {
                           if( algorithm::compare::any( code::system::last::error(), std::errc::resource_unavailable_try_again, std::errc::operation_would_block))
                              return {};

                           code::system::raise( "accept");
                        }

                        Socket socket{ strong::socket::id{ result}};
                        socket.set( socket::option::no_delay{});
                        // make sure we honour keepalive.
                        socket.set( communication::socket::option::keepalive< true>{});
                        return socket;
                     }
                  } // socket

               } // <unnamed>

            } // local

            std::string_view Address::host() const
            {
               if( auto found = algorithm::find( m_address, ':'))
                  return std::string_view( m_address.data(), std::distance( std::begin( m_address), std::begin( found)));

               return { m_address};
            }

            std::string_view Address::port() const
            {
               if( auto found = algorithm::find( m_address, ':'))
               {
                  ++found;
                  return std::string_view( found.data(), found.size());
               }
               return {};
            }

            namespace socket
            {
               namespace address
               {
                  Address host( const strong::socket::id descriptor)
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

                  Address peer( const strong::socket::id descriptor)
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

            namespace non::blocking
            {
               Socket connect( const Address& address)
               {
                  Trace trace( "common::communication::tcp::non::blocking::connect");

                  try
                  {
                     return tcp::connect( address);
                  }
                  catch( ...)
                  {
                     // if refused we return 'nil' socket
                     if( exception::code() != code::casual::communication_refused)
                        throw;
                  }
                  return {};

               }
            } // non::blocking

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
                     catch( ...)
                     {
                        // if refused : no-op - we go to sleep
                        if( exception::code() != code::casual::communication_refused)
                           throw;
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

                     ssize_t send( const strong::socket::id descriptor, const void* const data, platform::size::type const size, common::Flags< Flag> flags)
                     {
                        return posix::result( 
                           ::send( descriptor.value(), data, size, flags.underlaying()));
                     }

                     char* receive(
                           const strong::socket::id descriptor,
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
                              code::raise::log( code::casual::communication_unavailable);
                           }
                           common::log::line( verbose::log, "bytes: ", bytes); 

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
                              code::raise::error( code::casual::communication_protocol, "somehow more bytes was sent over the socket than requested");

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
                     local_send( socket.descriptor(), complete.payload.data(), complete.payload.data() + complete.payload.size(), flags);

                     log::line( log, "tcp send ---> socket: ", socket, ", complete: ", complete);

                     return complete.correlation;
                  }
                  catch( ...)
                  {
                     if( exception::code() == code::casual::communication_refused)
                        return {};

                     throw;
                  }
               }

               communication::message::Complete receive( const Socket& socket, common::Flags< Flag> flags)
               {
                  Trace trace{ "tcp::native::receive"};

                  try
                  {
                     communication::message::complete::network::Header header{};

                     auto current = reinterpret_cast< char*>( &header);

                     // First we get the header
                     {
                        const auto header_end = current + communication::message::complete::network::header::size();

                        local::receive( socket.descriptor(), current, header_end, flags);
                     }

                     // Now we can get the payload

                     communication::message::Complete message{ header};

                     // make sure we always block when we wait for the payload.
                     local::receive( socket.descriptor(), message.payload.data(), message.payload.data() + message.payload.size(), flags - Flag::non_blocking);

                     log::line( log, "tcp receive <---- socket: ", socket, " , complete: ", message);

                     return message;
                  }
                  catch( ...)
                  {
                     if( exception::code() == code::casual::communication_retry)
                        return {};

                     throw;
                  }
               }

               namespace local
               {
                  namespace
                  {
                     policy::cache_range_type receive( const Connector& tcp, policy::cache_type& cache, common::Flags< Flag> flags)
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

               cache_range_type basic_blocking::receive( const Connector& tcp, cache_type& cache)
               {
                  return native::local::receive( tcp, cache, {});
               }

               Uuid basic_blocking::send( const Connector& tcp, const communication::message::Complete& complete)
               {
                  return native::send( tcp.socket(), complete, {});
               }


               namespace non
               {
                  cache_range_type basic_blocking::receive( const Connector& tcp, cache_type& cache)
                  {
                     return native::local::receive( tcp, cache, native::Flag::non_blocking);
                  }

                  Uuid basic_blocking::send( const Connector& tcp, const communication::message::Complete& complete)
                  {
                     return native::send( tcp.socket(), complete, native::Flag::non_blocking);
                  }

               } // non

            } // policy

            Connector::Connector() noexcept = default;

            Connector::Connector( Socket&& socket) noexcept
               : m_socket( std::move( socket))
            {

            }

            Connector::Connector( const Socket& socket)
               : m_socket{ socket}
            {

            }

         } // tcp
      } // communication
   } // common
} // casual
