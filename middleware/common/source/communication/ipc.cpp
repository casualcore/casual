//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/communication/ipc.h"
#include "common/communication/log.h"

#include "common/result.h"
#include "common/log.h"
#include "common/signal.h"
#include "common/environment.h"

#include "common/exception/handle.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


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
                  return out << "{ type: " << value.type()
                     << ", correlation: " << uuid::string( value.correlation())
                     << ", offset: " << value.payload_offset()
                     << ", payload.size: " << value.payload_size()
                     << ", complete_size: " << value.complete_size()
                     << ", header-size: " << transport::header_size()
                     << ", transport-size: " <<  value.size()
                     << ", max-size: " << transport::max_message_size() << "}";
               }
            } // message

            Handle::Handle( Socket&& socket, strong::ipc::id ipc) : m_socket( std::move( socket)), m_ipc( ipc)
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

            std::ostream& operator << ( std::ostream& out, const Handle& rhs)
            {
               return out << "{ ipc: " << rhs.m_ipc << ", socket: " << rhs.m_socket << '}';
            }


            Address::Address( strong::ipc::id ipc)
            {
               const auto& directory = environment::transient::directory();
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
                     bool check_error()
                     {
                        auto error = code::last::system::error();
                        switch( error)
                        {
                           case code::system::resource_unavailable_try_again:
                           //TODO: case code::system::operation_would_block: 
                           {
                              // return false
                              break;
                           }
                           default: 
                           {
                              // will allways throw
                              exception::system::throw_from_code( error);
                           }
                        }
                        return false;
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


               bool send( const Socket& socket, const Address& destination, const message::Transport& transport, Flag flag)
               {
                  Trace trace{ "common::communication::ipc::native::send"};

                  log::line( verbose::log, "---> send - socket: ", socket, ", destination: ", destination, ", transport: ", transport, ", flags: ", flag);

                  while( true)
                  {
                     auto error = posix::error(
                        ::sendto(
                           socket.descriptor().value(), 
                           transport.data(), 
                           transport.size(),
                           cast::underlying( flag), //| cast::underlying( platform::flag::msg::no_signal), 
                           destination.native_pointer(),
                           destination.native_size())
                     );

                     if( ! error)
                        return true;
                  
                     switch( error.value())
                     {
                        case code::system::resource_unavailable_try_again:
                        //TODO: case code::system::operation_would_block: 
                        {
                           return false;
                        }
                        case code::system::no_buffer_space:
                        {
                           if( flag == Flag::non_blocking)
                              return false;
                           // we try to "wait" for buffer-space
                           process::sleep( std::chrono::milliseconds{ 1});
                           // try again...
                           break;
                        }
                        case code::system::no_such_file_or_directory:
                           throw exception::system::communication::unavailable::File{};   
                        default:
                        {
                           // will allways throw
                           exception::system::throw_from_code( error.value());
                        }
                     }
                  }
                  return true;
               }

               bool receive( const Handle& handle, message::Transport& transport, Flag flag)
               {
                  Trace trace{ "common::communication::ipc::native::receive"};

                  // make sure we can read to the socket...
                  if( flag != Flag::non_blocking)
                  {
                     Trace trace{ "common::communication::ipc::native::receive select"};
                     
                     fd_set read;
                     {
                        FD_ZERO( &read);
                        FD_SET( handle.socket().descriptor().value(), &read);
                     }

                     // block all signals
                     signal::thread::scope::Block block;

                     log::line( verbose::log, "signal blocked: ", block.previous());
                     
                     // check pending signals
                     signal::handle( block.previous());

                     // will set previous signal mask atomically
                     posix::result( 
                        ::pselect( handle.socket().descriptor().value() + 1, &read, nullptr, nullptr, nullptr, &block.previous().set),
                        block.previous());
                  }

                  auto result = ::recv(
                     handle.socket().descriptor().value(),
                     transport.data(),
                     message::transport::max_message_size(),
                     cast::underlying( flag)); // | cast::underlying( platform::flag::msg::no_signal));

                  if( result == -1)
                  {
                     return local::check_error();
                  }

                  log::line( verbose::log, "<--- receive - socket: ", handle, ", transport: ", transport, ", flags: ", flag);

                  assert( result == transport.size());

                  return true;
               }

            } // native


            namespace policy
            {
               cache_range_type receive( Handle& handle, cache_type& cache, native::Flag flag)
               {
                  message::Transport transport;

                  if( native::receive( handle, transport, flag))
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

               Uuid send( const Socket& socket, const Address& destination, const communication::message::Complete& complete, native::Flag flag)
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

                     //
                     // send the physical message
                     //
                     if( ! native::send( socket, destination, transport, flag))
                     {
                        return uuid::empty();
                     }

                     part_begin = part_end;
                  }
                  while( part_begin != std::end( complete.payload));

                  return complete.correlation;

               }

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

               std::ostream& operator << ( std::ostream& out, const Connector& rhs)
               {
                  return out << "{ handle: " << rhs.m_handle
                     << '}';
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

               std::ostream& operator << ( std::ostream& out, const Connector& rhs)
               {
                  return out << "{ destination: " << rhs.m_destination
                     << '}';
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