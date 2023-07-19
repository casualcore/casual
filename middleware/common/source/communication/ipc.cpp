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
#include "common/file.h"

#include "common/exception/capture.h"
#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/code/system.h"
#include "common/code/convert.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <utility>


namespace casual
{
   namespace common::communication::ipc
   {

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

      std::filesystem::path path( const strong::ipc::id& id)
      {
         constexpr auto max_size = sizeof( ::sockaddr_un::sun_path) -1;

         auto result = environment::directory::ipc() / uuid::string( id.value());

         if( result.native().size() > max_size)
            code::raise::error( code::casual::invalid_path, "transient directory path too long");

         return result;
      }


      Address::Address( strong::ipc::id ipc)
      {
         const auto path = ipc::path( ipc);

         auto target = std::begin( m_native.sun_path);

         algorithm::copy( path.native(), target);

         m_native.sun_family = AF_UNIX;
      }

      std::ostream& operator << ( std::ostream& out, const Address& rhs)
      {
         return out << "{ path: " << rhs.m_native.sun_path << '}';
      }

      namespace native
      {
         namespace local
         {
            namespace
            {
               bool check_error( std::errc code)
               {
                  switch( code)
                  {

#ifdef __APPLE__
                     case std::errc::no_buffer_space:
                     {
                        // try again...
                        std::this_thread::sleep_for( std::chrono::microseconds{ 100});
                        return false;
                     }
#endif
                     case std::errc::resource_unavailable_try_again:
                        return false;

#if EAGAIN != EWOULDBLOCK
                     case std::errc::operation_would_block:
                        return false;
#endif 
                  
                     case std::errc::no_such_file_or_directory:
                        code::raise::error( code::casual::communication_unavailable, "ipc - errno: ", code);

                     default:
                        // will always throw
                        code::raise::error( code::convert::to::casual( code), "ipc - errno: ", code);
                  }
               }

               bool check_error()
               {
                  return check_error( code::system::last::error());
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
                     auto result = Socket{ strong::socket::id{ posix::result( ::socket(AF_UNIX, SOCK_DGRAM, 0), "::socket(AF_UNIX, SOCK_DGRAM, 0)")}};
                     result.set( socket::option::File::close_in_child);
                     return result;
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
                        0, //| std::to_underlying( platform::flag::msg::no_signal), 
                        destination.native_pointer(),
                        destination.native_size())
                  );

                  if( ! error)
                  {
                     log::line( verbose::log, "ipc ---> blocking send - socket: ", socket, ", destination: ", destination, ", transport: ", transport);
                     return true;
                  }

                  local::check_error( std::errc{ error.value()});
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
                  0); // | std::to_underlying( platform::flag::msg::no_signal));

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
                        std::to_underlying( Flag::non_blocking), //| std::to_underlying( platform::flag::msg::no_signal), 
                        destination.native_pointer(),
                        destination.native_size())
                  );

                  if( error)
                     return local::check_error( std::errc{ error.value()});

                  log::line( verbose::log, "ipc ---> non blocking send - socket: ", socket, ", destination: ", destination, ", transport: ", transport);
                  return true;
               }

               bool receive( const Handle& handle, message::Transport& transport)
               {
                  Trace trace{ "common::communication::ipc::native::non::blocking::receive"};

                  auto result = ::recv(
                     handle.socket().descriptor().value(),
                     transport.data(),
                     message::transport::max_message_size(),
                     std::to_underlying( Flag::non_blocking)); // | std::to_underlying( platform::flag::msg::no_signal));

                  if( result == -1)
                     return local::check_error();

                  log::line( verbose::log, "ipc <--- non blocking receive - handle: ", handle, ", transport: ", transport);

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
                        [&]( auto& complete)
                        {
                           return ! complete.complete() 
                              && transport.type() == complete.type()
                              && transport.message.header.correlation == complete.correlation();
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
                           strong::correlation::id{ common::Uuid{ transport.correlation()}},
                           transport.complete_size(),
                           transport);

                     return cache_range_type{ std::end( cache) - 1, std::end( cache)};
                  }
               }
               return cache_range_type{};
            }

            strong::correlation::id send( const Socket& socket, const Address& destination, const ipc::message::Complete& complete)
            {
               message::Transport transport{ complete.type(), complete.size(), complete.correlation()};

               auto part_begin = std::begin( complete.payload);

               do
               {
                  auto part_end = std::distance( part_begin, std::end( complete.payload)) > message::transport::max_payload_size() ?
                        part_begin + message::transport::max_payload_size() : std::end( complete.payload);

                  transport.assign( range::make( part_begin, part_end));
                  transport.message.header.offset = std::distance( std::begin( complete.payload), part_begin);

                  // send the physical message
                  if( ! native::blocking::send( socket, destination, transport))
                     return {};

                  part_begin = part_end;
               }
               while( part_begin != std::end( complete.payload));

               return complete.correlation();
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
                           [&]( auto& complete)
                           {
                              return ! complete.complete()
                                 && transport.type() == complete.type()
                                 && transport.message.header.correlation == complete.correlation();
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
                              strong::correlation::id{ common::Uuid{ transport.correlation()}},
                              transport.complete_size(),
                              transport);

                        return cache_range_type{ std::end( cache) - 1, std::end( cache)};
                     }
                  }
                  return cache_range_type{};
               }

               strong::correlation::id send( const Socket& socket, const Address& destination, const ipc::message::Complete& complete)
               {
                  message::Transport transport{ complete.type(), complete.size(), complete.correlation()};

                  complete.correlation().value().copy( transport.correlation());

                  auto part_begin = std::begin( complete.payload);

                  do
                  {
                     auto part_end = std::distance( part_begin, std::end( complete.payload)) > message::transport::max_payload_size() ?
                           part_begin + message::transport::max_payload_size() : std::end( complete.payload);

                     transport.assign( range::make( part_begin, part_end));
                     transport.message.header.offset = std::distance( std::begin( complete.payload), part_begin);

                     // send the physical message
                     if( ! native::non::blocking::send( socket, destination, transport))
                        return {};

                     part_begin = part_end;
                  }
                  while( part_begin != std::end( complete.payload));

                  return complete.correlation();
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

                     directory::shared::create( environment::directory::ipc());

                     posix::result( ::bind(
                        socket.descriptor().value(),
                        address.native_pointer(),
                        address.native_size()
                     ), "bind socket: ", socket, " to address: ", address);

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
            if( m_handle)
            {
               exception::guard( [&]()
               {
                  ipc::remove( m_handle.ipc());
               });
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

      namespace partial
      {
         bool send( const Destination& destination, message::complete::Send& complete)
         {
            Trace trace{ "common::communication::ipc::outbound::partial::send"};
            log::line( verbose::log, "destination: ", destination, ", complete: ", complete);

            auto transport = complete.transport();

            while( auto front = complete.front())
            {
               transport.assign( front.range, front.offset);
               log::line( verbose::log, "transport: ", transport);

               if( ! native::non::blocking::send( destination.socket(), destination.address(), transport))
                  return false;

               complete.pop();
            }
            return true;
         }
         
      } // partial

      namespace local
      {
         namespace
         {
            bool exists( const std::filesystem::path& path) noexcept
            {
               log::line( verbose::log, "path: ", path);
               return std::filesystem::exists( path);
            }
            
         } // <unnamed>
      } // local

      bool exists( strong::ipc::id id)
      {
         Trace trace{ "common::communication::ipc::exists"};
         return local::exists( ipc::path( id));
      }

      bool remove( strong::ipc::id id)
      {
         Trace trace{ "common::communication::ipc::remove"};

         auto path = ipc::path( id);

         if( ! local::exists( path))
            return false;

         return std::filesystem::remove( path);
      }

      bool remove( const process::Handle& owner)
      {
         return remove( owner.ipc);
      }

   } // common::communication::ipc
} // casual
