//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/communication/tcp.h"
#include "common/communication/log.h"
#include "common/communication/select.h"

#include "common/result.h"

#include "common/code/convert.h"
#include "common/code/raise.h"
#include "common/code/system.h"
#include "common/code/casual.h"
#include "common/string/compose.h"
#include "common/exception/compose.h"

#include "common/log.h"
#include "common/functional.h"

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
   namespace common::communication::tcp
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

               //! indicate that the enum is used as a flag (g++11 bug (?) force to define it)
               [[maybe_unused]] consteval void casual_enum_as_flag( Flag) {};

               namespace address
               {
                  struct Native 
                  {
                     explicit Native( const tcp::Address& address, Flag flags = {})
                     {
                        Trace trace( "common::communication::tcp::local::socket::address::Native::Native");
                        log::line( verbose::log, "address: ", address, ", flags: ", flags);

                        ::addrinfo hints{};

                        // IPV4 or IPV6 doesn't matter
                        hints.ai_family = PF_UNSPEC;
                        // TCP/IP
                        hints.ai_socktype = SOCK_STREAM;

                        flags |= Flag::canonical;
                        hints.ai_flags = std::to_underlying( flags);

                        std::string host{ address.host()};
                        std::string port{ address.port()};


                        // if not successful some errors will raise, the rest we log and keep the address::Native in an invalid state.
                        if( const int result = ::getaddrinfo( host.data(), port.data(), &hints, &m_information.value))
                        {
                           auto compose_error = []( auto error)
                           {
                              return string::compose( "[", error, ':', ::gai_strerror( error), ']');
                           };

                           switch( result)
                           {
                              // non fatal
                              case EAI_ADDRFAMILY:
                              case EAI_AGAIN:
                              case EAI_NODATA:
                              case EAI_NONAME:
                                 log::line( verbose::log, "address: ", address, " - recoverable error: ", compose_error( result));
                                 break;

                              // fatal
                              case EAI_BADFLAGS:
                              case EAI_FAIL:
                              case EAI_FAMILY:
                              case EAI_MEMORY:
                              case EAI_SERVICE:
                              case EAI_SOCKTYPE:
                                 code::raise::error( code::casual::communication_invalid_address, "address: ", address, " - fatal error: ", compose_error( result));
                              case EAI_SYSTEM:
                                 code::system::raise();
                              default:
                                 code::raise::error( code::casual::internal_unexpected_value, "unexpected result from getaddrinfo: ", compose_error( result));
                                 
                           }
                        }  
                     }

                     Native() = default;

                     ~Native()
                     {
                        if( m_information)
                           freeaddrinfo( m_information.value);
                     }

                     Native( Native&&) noexcept = default;
                     Native& operator = ( Native&&) noexcept = default;

                     explicit operator bool() const noexcept { return predicate::boolean( m_information);}

                     struct iterator
                     {
                        using difference_type = platform::size::type;

                        iterator() = default;
                        iterator( const addrinfo* data) : data{ data} {}

                        //auto& operator * () const { return *data;}
                        inline auto& operator * () const { return *data;}
                        inline auto operator -> () const { return data;}

                        iterator& operator ++ () { data = data->ai_next; return *this;}
                        iterator operator ++ ( int) { iterator result{ data}; data = data->ai_next; return result;}

                        friend bool operator == ( iterator lhs, iterator rhs) { return lhs.data == rhs.data;}
                        friend bool operator != ( iterator lhs, iterator rhs) { return ! ( lhs == rhs);}
                        const addrinfo* data = nullptr;
                     };

                     auto begin() const { return iterator{ m_information.value};}
                     auto end() const { return iterator{};}

                     bool empty() const noexcept { return m_information.value == nullptr;}

                  private:

                     common::move::Pointer< addrinfo> m_information;
                  };

                  static_assert( std::input_or_output_iterator < Native::iterator>); 

                  static_assert( concepts::range< Native>);

               } // address

               template< typename F>
               auto create( const Address& address, F binder, Flag flags = {})
                  -> decltype( binder( Socket{}, std::declval< const addrinfo&>()))
               {
                  Trace trace( "common::communication::tcp::local::socket::create");

                  address::Native native{ address, flags};

                  if( ! native)
                     return Socket{};

                  log::line( verbose::log, "native: ", native);

                  for( auto& info : native)
                  {
                     auto socket = Socket{ 
                        strong::socket::id{ ::socket( info.ai_family, info.ai_socktype, info.ai_protocol)}};

                     if( socket)
                     {
                        socket.set( local::socket::option::no_delay{});
                        // make sure we honour keepalive
                        socket.set( communication::socket::option::keepalive< true>{});

                        return binder( std::move( socket), info);
                     }
                  }
                  return {};
               }

               auto connect( const Address& address)
               {
                  Trace trace( "common::communication::tcp::local::socket::connect");

                  // We block all signals while we're doing one connect attempt
                  //common::signal::thread::scope::Block block;

                  return create( address, []( Socket socket, const addrinfo& info) 
                     -> std::variant< Socket, non::blocking::Pending, std::system_error>
                  {
                     Trace trace( "common::communication::tcp::local::socket::connect lambda");

                     // To avoid possible TIME_WAIT from previous possible connections
                     socket.set( communication::socket::option::reuse_address< true>{});
                     socket.set( communication::socket::option::linger{ std::chrono::seconds{ 1}});

                     // make sure we don't block
                     socket.set( communication::socket::option::File::no_block);

                     if( ::connect( socket.descriptor().value(), info.ai_addr, info.ai_addrlen) == 0)
                        return socket;

                     auto error = code::system::last::error();

                     if( error == std::errc::operation_in_progress)
                        return non::blocking::Pending{ std::move( socket)};

                     log::line( verbose::log, "error: ", error);

                     if( non::blocking::error::recoverable( error))
                        return Socket{};

                     return exception::compose( code::convert::to::casual( error), "errno: ", error, ", socket: ", socket, ", info: ", info);
                     
                  });
               }

               auto bind( const Address& address)
               {
                  Trace trace( "common::communication::tcp::local::socket::bind");

                  // We block all signals while we're trying to set up a listener...
                  //common::signal::thread::scope::Block block;

                  constexpr auto flags = Flag::address_config | Flag::passive;

                  return create( address,[]( Socket socket, const addrinfo& info) -> Socket
                  {
                     Trace trace( "common::communication::tcp::local::socket::bind lambda");

                     // To avoid possible TIME_WAIT from previous
                     // possible connections
                     // This might not get desired results though
                     // Checkout SO_LINGER as well
                     socket.set( communication::socket::option::reuse_address< true>{});
                     socket.set( communication::socket::option::linger{ std::chrono::seconds{ 1}});

                     // TODO what about possible errors?
                     if( ::bind( socket.descriptor().value(), info.ai_addr, info.ai_addrlen) == -1)
                        return {};
                     
                     return socket;
                  }, flags);
               }

               Address names( const struct sockaddr& info, const socklen_t size)
               {
                  char host[ NI_MAXHOST];
                  char serv[ NI_MAXSERV];
                  const int flags{ NI_NUMERICHOST | NI_NUMERICSERV};

                  posix::result(
                     ::getnameinfo(
                        &info, size,
                        host, NI_MAXHOST,
                        serv, NI_MAXSERV,
                        flags), "::getnameinfo - ", __FILE__, ':', __LINE__);

                  return { string::compose( host, ':', serv)};
               }

               Socket accept( const strong::socket::id descriptor)
               {
                  auto result = strong::socket::id{ ::accept( descriptor.value(), nullptr, nullptr)};

                  if( ! result)
                  {
                     if( algorithm::compare::any( code::system::last::error(), std::errc::resource_unavailable_try_again, std::errc::operation_would_block))
                        return {};

                     code::system::raise( "accept");
                  }

                  Socket socket{ std::move( result)};
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
            Address host( const strong::socket::id descriptor) noexcept
            {
               try
               {
                  ::sockaddr info{};
                  ::socklen_t size = sizeof( info);
                  posix::result( ::getsockname( descriptor.value(), &info, &size));
                  return local::socket::names( info, size);
               }
               catch( ...)
               {
                  log::line( communication::log, exception::capture(), " - failed to get host address from: ", descriptor);
                  return {};
               }
            }

            Address host( const Socket& socket) noexcept
            {
               return host( socket.descriptor());
            }

            Address peer( const strong::socket::id descriptor) noexcept
            {
               try
               {
                  ::sockaddr info{};
                  ::socklen_t size = sizeof( info);
                  posix::result( ::getpeername( descriptor.value(), &info, &size));
                  return local::socket::names( info, size);
               }
               catch( ...)
               {
                  log::line( communication::log, exception::capture(), " - failed to get peer address from: ", descriptor);
                  return {};
               }
            }

            Address peer( const Socket& socket) noexcept
            {
               return peer( socket.descriptor());
            }

         } // address

         Socket listen( const Address& address)
         {
            Trace trace( "common::communication::tcp::socket::listen");

            auto result = local::socket::bind( address);

            // queuesize could (probably) be set to zero as well (in casual-context)
            if( auto error = posix::error( ::listen( result.descriptor().value(), platform::tcp::listen::backlog)))
               code::raise::error( code::casual::communication_invalid_address, "failed to create listener socket - ", error);

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

         return std::visit( overload::compose(
            []( Socket socket){ return socket;},
            []( const std::system_error& error) 
            {
               log::line( verbose::log, "fatal error: ", error); 
               throw error;
               // dummy return that will never be returned...
               // TODO c++20 add [[noreturn]] (not sure if std::visit will "understand" though..)
               return Socket{};
            },
            []( non::blocking::Pending pending) -> Socket
            {
               Trace trace( "common::communication::tcp::connect pending");

               auto socket = std::move( pending).socket();

               // wait until ready
               select::block::write( socket.descriptor());

               if( auto error = socket.error())
               {
                  if( non::blocking::error::recoverable( error.value()))
                     return {};
                  else
                     exception::compose( error.value(), "fatal error on connect");
               }

               return socket;
            }
         ), local::socket::connect( address));
      }

      namespace non::blocking
      {
         namespace error
         {
            bool recoverable( std::errc error) noexcept
            {
               switch( error)
               {
                  case std::errc::operation_in_progress:
                  case std::errc::already_connected: // will not happen
                  case std::errc::connection_refused:
                  case std::errc::connection_reset:
                  case std::errc::address_not_available:
                  case std::errc::address_in_use:
                  case std::errc::address_family_not_supported:
                  case std::errc::network_unreachable:
                  case std::errc::network_down:
                  case std::errc::timed_out:
                  case std::errc::host_unreachable:
                     return true;
                  default:
                     return false;
               }
            }
            
         } // error

         std::variant< Socket, Pending, std::system_error> connect( const Address& address) noexcept
         {
            Trace trace( "common::communication::tcp::non::blocing::connect");
            try
            {
               return local::socket::connect( address);
            }
            catch( ...)
            {
               return exception::capture();
            }
         }

      } // non::blocking


      Listener::Listener( Address address) : m_listener{ socket::listen( address)}
      {}

      Socket Listener::operator() () const
      {
         Trace trace( "common::communication::tcp::Listener::operator()");

         // make sure we "safely" block and wait for a connection
         communication::select::block::read( m_listener.descriptor());

         return socket::accept( m_listener);
      }


      namespace policy
      {

         namespace local
         {
            namespace
            {

               auto send( const Socket& socket, policy::complete_type& complete)
               {
                  Trace trace{ "common::communication::tcp::policy::local::send"};
                  log::line( verbose::log, "complete: ", complete);

                  try
                  {
                     auto send_message = []( auto& socket, const ::msghdr* message)
                     {
                        log::line( verbose::log, "socket: ", socket);

                        return posix::alternative( 
                           ::sendmsg( socket.descriptor().value(), message, 0),
                           0,
                           std::errc::resource_unavailable_try_again, std::errc::operation_would_block);
                     };

                     if( complete.offset >= message::header::size)
                     {
                        // we've sent the header already. Send the rest of the payload.

                        ::iovec part{};

                        auto offset = complete.offset - message::header::size;

                        part.iov_base = const_cast< char*>( complete.payload.data()) + offset;
                        part.iov_len = complete.payload.size() - offset;

                        ::msghdr message{};
                        message.msg_iov = &part;
                        message.msg_iovlen = 1;

                        if( auto count = send_message( socket, &message))
                        {
                           complete.offset += count;
                           log::line( log::category::event::message::part::sent, complete.type(), '|', complete.correlation(), '|', count, '|', complete.offset - count, '|', complete.size());
                        }
                        
                     }
                     else
                     {
                        // part/all of the header is not sent. 
                        std::array< ::iovec, 2> parts{};

                        // First we set the header
                        parts[ 0].iov_base = complete.header_data() + complete.offset;
                        parts[ 0].iov_len = message::header::size - complete.offset;

                        // then we set the payload
                        parts[ 1].iov_base = complete.payload.data();
                        parts[ 1].iov_len = complete.payload.size();

                        ::msghdr message{};
                        message.msg_iov = parts.data();
                        message.msg_iovlen = parts.size();

                        if( auto count = send_message( socket, &message))
                        {
                           complete.offset += count;
                           log::line( log::category::event::message::part::sent, complete.type(), '|', complete.correlation(), '|', count, '|', complete.offset - count, '|', complete.size());
                        }
                     }

                     log::line( log, "tcp send ---> descriptor: ", socket.descriptor(), ", complete: ", complete);

                     return complete.complete();
                  }
                  catch( ...)
                  {
                     if( exception::capture().code() != code::casual::communication_refused)
                        throw;
   
                     return false;

                  }
               }

               auto receive( const Socket& socket, policy::complete_type& complete)
               {
                  Trace trace{ "common::communication::tcp::policy::local::receive"};
                  log::line( verbose::log, "socket: ", socket);
                  log::line( verbose::log, "complete: ", complete);

                  auto receive_message = []( auto& socket, auto first, auto count)
                  {
                     auto bytes = posix::alternative( 
                        ::recv( socket.descriptor().value(), first, count, 0),
                        -1,
                        std::errc::resource_unavailable_try_again, std::errc::operation_would_block);

                     // _The return value will be 0 when the peer has performed an orderly shutdown_
                     if( bytes == 0)
                        code::raise::error( code::casual::communication_unavailable);
                     else if( bytes == -1)
                        return 0L;

                     return bytes;
                  };

                  if( complete.offset < message::header::size)
                  {
                     // we receive the header (or the rest of it)
                     auto first = complete.header_data() + complete.offset;
                     auto count = receive_message( socket, first, message::header::size - complete.offset);
                     complete.offset += count;

                     if( complete.offset == message::header::size)
                        complete.payload.resize( complete.size());

                     log::line( log::category::event::message::part::received, complete.type(), '|', complete.correlation(), '|', count, '|', complete.offset - count, '|', complete.size());
                  }

                  if( complete.offset >= message::header::size)
                  {
                     // we receive the payload (or the rest of it).
                     auto offset = complete.offset - message::header::size;
                     auto first = complete.payload.data() + offset;
                     auto count = receive_message( socket, first, complete.payload.size() - offset);
                     
                     complete.offset += count;

                     log::line( log::category::event::message::part::received, complete.type(), '|', complete.correlation(), '|', count, '|', complete.offset - count, '|', complete.size());
                  }

                  log::line( log, "tcp receive <---- descriptor: ", socket.descriptor(), " , complete: ", complete);

                  return complete.complete();
               }


               policy::cache_range_type receive( const Socket& socket, policy::cache_type& cache)
               {
                  Trace trace{ "common::communication::tcp::policy::local::receive cache"};

                  auto range_last = []( auto& range)
                  {
                     return policy::cache_range_type{ std::end( range) - 1, std::end( range)};
                  };

                  // check if we got anything in the cache we need to take care of.

                  if( ! cache.empty() && ! cache.back().complete())
                  {
                     if( receive( socket, cache.back()))
                        return range_last( cache);
                  }
                  else
                  {
                     policy::complete_type complete;
                     if( receive( socket, complete))
                     {
                        cache.push_back( std::move( complete));
                        return range_last( cache);
                     }
                     else if( ! complete.empty())
                     {
                        // we got part of the message, put in the cache...
                        cache.push_back( std::move( complete));
                     }
                  }

                  return {};
               }

            } // <unnamed>
         } // local

         cache_range_type Blocking::receive( Connector& tcp, cache_type& cache)
         {
            tcp.socket().unset( communication::socket::option::File::no_block);

            while( true)
               if( auto result = local::receive( tcp.socket(), cache))
                  return result;
         }

         strong::correlation::id Blocking::send( Connector& tcp, complete_type&& complete)
         {
            tcp.socket().unset( communication::socket::option::File::no_block);

            while( ! local::send( tcp.socket(), complete))
               ; // no-op

            return complete.correlation();
         }


         namespace non
         {
            cache_range_type Blocking::receive( Connector& tcp, cache_type& cache)
            {
               tcp.socket().set( communication::socket::option::File::no_block);
               return local::receive( tcp.socket(), cache);
            }

            strong::correlation::id Blocking::send( Connector& tcp, complete_type& complete)
            {
               tcp.socket().set( communication::socket::option::File::no_block);
               if( local::send( tcp.socket(), complete))
                  return complete.correlation();
               return {};
            }

         } // non

      } // policy

      Connector::Connector() noexcept = default;

      Connector::Connector( Socket&& socket) noexcept
         : m_socket( std::move( socket))
      {

      }


   } // common::communication::tcp
} // casual
