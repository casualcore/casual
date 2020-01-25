//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/ipc.h"
#include "common/communication/log.h"
#include "common/communication/select.h"

#include "common/result.h"
#include "common/log.h"
#include "common/signal.h"
#include "common/environment.h"

#include "common/exception/handle.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <utility>


namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace ipc
         {
            namespace message
            {
               static_assert( transport::max_message_size() <= platform::ipc::transport::size, "ipc message is too big'");

               std::ostream& operator << ( std::ostream& out, const Transport& value)
               {
                  return stream::write( out, 
                     "{ header: " , value.message.header
                     , ", payload.size: " , value.payload_size()
                     , ", header-size: " , transport::header_size()
                     , ", transport-size: " ,  value.size()
                     , ", max-size: " , transport::max_message_size() 
                     , '}');
               }
            } // message

            Handle::Handle( Socket&& socket, strong::ipc::id ipc) : m_socket( std::move( socket)), m_ipc(std::move( ipc))
            {
               log::line( communication::verbose::log, "created handle: ", *this);
            }

            Handle::Handle( Handle&& other) noexcept 
               : m_socket( std::exchange( other.m_socket, {})), 
                 m_ipc( std::exchange( other.m_ipc, {}))
            {

            }

            Handle& Handle::operator = ( Handle&& other) noexcept
            {
               std::swap( m_socket, other.m_socket);
               std::swap( m_ipc, other.m_ipc);
               return *this;
            }

            Handle::~Handle() = default;


            Address::Address( strong::ipc::id ipc)
            {
               const auto& directory = environment::ipc::directory();
               const auto postfix = '/' + uuid::string( ipc.value());
               const auto max_size = sizeof( m_native.sun_path) - postfix.size() - 1;

               if( directory.size() > max_size)
                  throw exception::system::invalid::Argument{ string::compose( "transient directory path to long - max size: ", max_size)};

               auto target = std::begin( m_native.sun_path);

               algorithm::copy( directory, target);
               algorithm::copy( postfix, target + directory.size());

               m_native.sun_family = AF_UNIX;
            }

            std::ostream& operator << ( std::ostream& out, const Address& rhs)
            {
               return out << "{ path: " << rhs.m_native.sun_path
                  << '}';
            }

            namespace native
            {
               namespace local
               {
                  namespace
                  {

                     bool check_error( code::system code)
                     {
                        switch( code)
                        {
                           case code::system::no_buffer_space:
                              // try again...
                              std::this_thread::sleep_for( std::chrono::microseconds{ 10});
                              break;

                           case code::system::no_such_file_or_directory:
                              throw exception::system::communication::unavailable::File{}; 

                           case code::system::resource_unavailable_try_again:
#if EAGAIN != EWOULDBLOCK
                           case code::system::operation_would_block: 
#endif                           
                              // return false
                              break;
                           
                           default: 
                           
                              // will allways throw
                              exception::system::throw_from_code( code);   
                        
                        }
                        return false;
                     }

                     bool check_error()
                     {
                        return check_error( code::last::system::error());
                     } 
                  } // <unnamed>
               } // local
               namespace detail
               {
                  namespace create
                  {
                     namespace domain
                     {
                        Socket socket()
                        {
                           return Socket{ strong::socket::id{ posix::result( ::socket(AF_UNIX, SOCK_DGRAM, 0))}};
                        }
                     } // domain
                  } // create

                  namespace outbound
                  {
                     const Socket& socket()
                     {
                        static const Socket singleton = create::domain::socket();
                        return singleton;
                     }
                  } // outbound
               } // detail

               namespace blocking
               {
                  bool send( const Socket& socket, const Address& destination, const message::Transport& transport)
                  {
                     Trace trace{ "common::communication::ipc::native::blocking::send"};

                     // The loop is "only" for BSD/OS X crap handling
                     while( true)
                     {
                        auto error = posix::error(
                           ::sendto(
                              socket.descriptor().value(), 
                              transport.data(), 
                              transport.size(),
                              0, //| cast::underlying( platform::flag::msg::no_signal), 
                              destination.native_pointer(),
                              destination.native_size())
                        );

                        if( ! error)
                        {
                           log::line( verbose::log, "ipc ---> blocking send - socket: ", socket, ", destination: ", destination, ", transport: ", transport);
                           return true;
                        }

                        local::check_error();
                        log::line( verbose::log, "ipc ---> blocking send - error: ", error, ", destination: ", destination);
                     }
                  }

                  bool receive( const Handle& handle, message::Transport& transport)
                  {
                     Trace trace{ "common::communication::ipc::native::blocking::receive"};

                     // first we try non-blocking
                     if( non::blocking::receive( handle, transport))
                        return true;

                     select::block::read( handle.socket().descriptor());

                     auto result = ::recv(
                        handle.socket().descriptor().value(),
                        transport.data(),
                        message::transport::max_message_size(),
                        0); // | cast::underlying( platform::flag::msg::no_signal));

                     if( result == -1)
                        return local::check_error();

                     log::line( verbose::log, "ipc <--- blocking receive - handle: ", handle, ", transport: ", transport);
                     assert( result == transport.size());

                     return true;                     
                  }
                  
               } // blocking

               namespace non
               {
                  namespace blocking
                  {

                     bool send( const Socket& socket, const Address& destination, const message::Transport& transport)
                     {
                        Trace trace{ "common::communication::ipc::native::non::blocking::send"};

                        auto error = posix::error(
                           ::sendto(
                              socket.descriptor().value(), 
                              transport.data(), 
                              transport.size(),
                              cast::underlying( Flag::non_blocking), //| cast::underlying( platform::flag::msg::no_signal), 
                              destination.native_pointer(),
                              destination.native_size())
                        );

                        if( error)
                           return local::check_error( error.value());

                        log::line( verbose::log, "---> non blocking send - socket: ", socket, ", destination: ", destination, ", transport: ", transport);
                        return true;
                     }

                     bool receive( const Handle& handle, message::Transport& transport)
                     {
                        Trace trace{ "common::communication::ipc::native::non::blocking::receive"};

                        auto result = ::recv(
                           handle.socket().descriptor().value(),
                           transport.data(),
                           message::transport::max_message_size(),
                           cast::underlying( Flag::non_blocking)); // | cast::underlying( platform::flag::msg::no_signal));

                        if( result == -1)
                           return local::check_error();

                        log::line( verbose::log, "<--- non blocking receive - handle: ", handle, ", transport: ", transport);

                        assert( result == transport.size());

                        return true;
                     }
                  } // blocking
               } // non

               bool send( const Socket& socket, const Address& destination, const message::Transport& transport, Flag flag)
               {
                  if( flag == Flag::non_blocking) 
                     return non::blocking::send( socket, destination, transport);
                  else
                     return blocking::send( socket, destination, transport);
               }

               bool receive( const Handle& handle, message::Transport& transport, Flag flag)
               {
                  if( flag == Flag::non_blocking) 
                     return non::blocking::receive( handle, transport);
                  else
                     return blocking::receive( handle, transport);
               }
            } // native


            namespace policy
            {
               namespace blocking
               {
                  cache_range_type receive( Handle& handle, cache_type& cache)
                  {
                     message::Transport transport;

                     if( native::blocking::receive( handle, transport))
                     {
                        auto found = algorithm::find_if( cache,
                              [&]( const auto& m)
                              {
                                 return ! m.complete() 
                                    && transport.message.header.correlation == m.correlation;
                              });

                        if( found)
                        {
                           found->add( transport);
                           return found;
                        }
                        else
                        {
                           cache.emplace_back(
                                 transport.type(),
                                 transport.correlation(),
                                 transport.complete_size(),
                                 transport);

                           return cache_range_type{ std::end( cache) - 1, std::end( cache)};
                        }
                     }
                     return cache_range_type{};
                  }

                  Uuid send( const Socket& socket, const Address& destination, const communication::message::Complete& complete)
                  {
                     message::Transport transport{ complete.type, complete.size()};

                     complete.correlation.copy( transport.correlation());

                     auto part_begin = std::begin( complete.payload);

                     do
                     {
                        auto part_end = std::distance( part_begin, std::end( complete.payload)) > message::transport::max_payload_size() ?
                              part_begin + message::transport::max_payload_size() : std::end( complete.payload);

                        transport.assign( range::make( part_begin, part_end));
                        transport.message.header.offset = std::distance( std::begin( complete.payload), part_begin);

                        // send the physical message
                        if( ! native::blocking::send( socket, destination, transport))
                           return uuid::empty();

                        part_begin = part_end;
                     }
                     while( part_begin != std::end( complete.payload));

                     return complete.correlation;
                  }

               } // blocking

               namespace non
               {
                  namespace blocking
                  {
                     cache_range_type receive( Handle& handle, cache_type& cache)
                     {
                        message::Transport transport;

                        if( native::non::blocking::receive( handle, transport))
                        {
                           auto found = algorithm::find_if( cache,
                                 [&]( const auto& m)
                                 {
                                    return ! m.complete() 
                                       && transport.message.header.correlation == m.correlation;
                                 });

                           if( found)
                           {
                              found->add( transport);
                              return found;
                           }
                           else
                           {
                              cache.emplace_back(
                                    transport.type(),
                                    transport.correlation(),
                                    transport.complete_size(),
                                    transport);

                              return cache_range_type{ std::end( cache) - 1, std::end( cache)};
                           }
                        }
                        return cache_range_type{};
                     }

                     Uuid send( const Socket& socket, const Address& destination, const communication::message::Complete& complete)
                     {
                        message::Transport transport{ complete.type, complete.size()};

                        complete.correlation.copy( transport.correlation());

                        auto part_begin = std::begin( complete.payload);

                        do
                        {
                           auto part_end = std::distance( part_begin, std::end( complete.payload)) > message::transport::max_payload_size() ?
                                 part_begin + message::transport::max_payload_size() : std::end( complete.payload);

                           transport.assign( range::make( part_begin, part_end));
                           transport.message.header.offset = std::distance( std::begin( complete.payload), part_begin);

                           // send the physical message
                           if( ! native::non::blocking::send( socket, destination, transport))
                           {
                              return uuid::empty();
                           }

                           part_begin = part_end;
                        }
                        while( part_begin != std::end( complete.payload));

                        return complete.correlation;
                     }
                  } // blocking
               } // non


            } // policy

            namespace inbound
            {
               namespace local
               {
                  namespace
                  {
                     namespace create
                     {
                        Handle handle()
                        {
                           auto socket = native::detail::create::domain::socket();

                           strong::ipc::id ipc{ uuid::make()};

                           Address address{ ipc};

                           posix::result( ::bind(
                              socket.descriptor().value(),
                              address.native_pointer(),
                              address.native_size()
                           ));

                           return Handle{ std::move( socket), ipc};
                        }
                     } // create
                  } // <unnamed>
               } // local

               Connector::Connector() 
                  : m_handle{ local::create::handle()}

               {
               }

               Connector::~Connector()
               {
                  try
                  {
                     if( m_handle)
                     {
                        ipc::remove( m_handle.ipc());
                     }
                  }
                  catch( ...)
                  {
                     exception::handle();
                  }
               }


               Device& device()
               {
                  static Device singleton;
                  return singleton;
               }
            } // inbound


            namespace outbound
            {
               Connector::Connector( strong::ipc::id ipc) 
                  : m_destination{ ipc}
               {
               }

            } // outbound


            bool exists( strong::ipc::id id)
            {
               const Address address{ id};
               return ::access( address.native().sun_path, F_OK) != -1; 
            }


            bool remove( strong::ipc::id id)
            {
               Address address{ id};
               return ::unlink( address.native().sun_path) != -1;
            }

            bool remove( const process::Handle& owner)
            {
               return remove( owner.ipc);
            }


         } // ipc
      } // communication
   } // common
} // casual
